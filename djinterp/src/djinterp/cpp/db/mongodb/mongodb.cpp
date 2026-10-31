/*******************************************************************************
* djinterp [cpp]                                                     mongodb.cpp
*
* Definitions for mongodb.hpp: mongo_connection's hooks and members, its
* cursors and change streams, and the MongoDB layer's failures, over the
* layer's face.
*   Every operation is one call of the face, whose result decides: 0 returns,
* anything else throws, through mongodb_layer::raise, with the result -- the
* server's code as it sent it, a failure of the driver's, libbson's or the
* layer's packed above it, or a negative D_DB_* -- as the exception's
* error_code(), and the server's, the driver's or the layer's own message
* where the handle recorded one about the failure. Connecting connects a
* face of its own, with the configuration's connection string, its options
* and its settings, so that a connection is either open with all of them
* applied or not open at all.
*   An empty filter, document, pipeline, options, database or bucket is the
* layer's NULL, none; a name is passed as it is, so that an empty one is the
* layer's to refuse.
*
*
* path:      /src/djinterp/cpp/db/mongodb/mongodb.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/db/mongodb/mongodb.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>    // std::size_t
#include <deque>      // std::deque
#include <exception>  // std::exception
#include <memory>     // std::unique_ptr, std::make_unique, std::shared_ptr
#include <optional>   // std::optional, std::nullopt
#include <string>     // std::string, std::to_string, std::stoll
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb.h"  // the layer
#include "../../../../../inc/djinterp/cpp/db/database_statement.hpp"
                                       // failure_kind, buffered_result_set
// re_std
#include "../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

/**
 * @brief The layer's argument for optional text: NULL for an empty string,
 *        which the layer reads as none.
 *
 * @param[in] _text  the text.
 * @return the argument.
 */
face::string_arg optional_arg(
    const std::string& _text
) noexcept
{
    return (_text.empty()) ? face::string_arg()
                           : face::string_arg(_text);
}

/**
 * @brief Writes a result's code as the message's end shows it: the server's
 *        code and its name, the domain and code of a failure the layer
 *        packed, or db_common.h's number.
 *
 * @param[in] _db      the handle the failure was recorded on; may be `NULL`.
 * @param[in] _result  the result.
 * @return the code, as text.
 */
std::string code_text(
    const struct dj_mongodb* _db,
    int                      _result
)
{
    // db_common.h's own, and nothing
    if (_result <= 0)
    {
        return std::to_string(_result);
    }

    const unsigned code = d_mongodb_result_code(_result);

    // the server's, named where the handle has its name
    if (!d_mongodb_result_is_client(_result))
    {
        const char* const name = ( (_db) &&
                                   (d_mongodb_error_code(_db) == _result) )
                                     ? d_mongodb_error_name(_db)
                                     : "";
        std::string       text = "server " + std::to_string(code);

        if ( (name) &&
             (name[0] != '\0') )
        {
            text += ", ";
            text += name;
        }

        return text;
    }

    const unsigned domain = d_mongodb_result_domain(_result);

    // the layer's, libbson's, the driver's
    if (domain >= D_MONGODB_DOMAIN_LAYER)
    {
        return "layer " + std::to_string(code);
    }

    if (domain >= D_MONGODB_DOMAIN_BSON)
    {
        return "libbson " + std::to_string(domain - D_MONGODB_DOMAIN_BSON) +
               "." + std::to_string(code);
    }

    return "driver " + std::to_string(domain) + "." + std::to_string(code);
}

// connect_settings
//   class: the settings d_mongodb_connect_with takes, with the text they
// point to kept alive beside them, in a std::deque whose elements never
// move. Not copyable.
class connect_settings
{
public:
    connect_settings() = default;

    connect_settings(
        const connect_settings&
    ) = delete;
    connect_settings& operator=(
        const connect_settings&
    ) = delete;

    // number -- a setting that reads a number: milliseconds, a switch
    void
    number(
        enum dj_mongodb_setting_name _name,
        unsigned long               _number
    )
    {
        m_settings.push_back({ _name, _number, nullptr, nullptr });

        return;
    }

    /**
     * @brief Adds a setting that reads text, and a number beside it.
     *
     * @param[in] _name    the setting.
     * @param[in] _text    its text.
     * @param[in] _number  its number; 0 where it reads none.
     * @throws connection_exception with D_DB_INVALID_ARGUMENT for text that
     *         holds a NUL.
     */
    void
    text(
        enum dj_mongodb_setting_name _name,
        const std::string&          _text,
        unsigned long               _number = 0ul
    )
    {
        m_settings.push_back({ _name, _number, m_keep(_text), nullptr });

        return;
    }

    /**
     * @brief Adds a connection string's option, by its name.
     *
     * @param[in] _option  the option's name.
     * @param[in] _value   its value, as the string would write it.
     * @throws connection_exception with D_DB_INVALID_ARGUMENT for text that
     *         holds a NUL.
     */
    void option(
        const std::string& _option,
        const std::string& _value
    )
    {
        const char* const name  = m_keep(_option);
        const char* const value = m_keep(_value);

        m_settings.push_back({ D_MONGODB_SETTING_URI_OPTION,
                               0ul,
                               name,
                               value });

        return;
    }

    const struct dj_mongodb_setting* data() const noexcept
    {
        return (m_settings.empty()) ? nullptr
                                    : m_settings.data();
    }

    std::size_t size() const noexcept
    {
        return m_settings.size();
    }

private:
    // m_keep -- a copy of the text, alive while the settings are, refused
    // where it holds a NUL
    const char* m_keep(
        const std::string& _text
    )
    {
        internal::refuse_nul_setting(_text, "MongoDB connect", "a setting");
        m_texts.push_back(_text);

        return m_texts.back().c_str();
    }

    std::deque<std::string>               m_texts;
    std::vector<struct dj_mongodb_setting> m_settings;
};

// auth_mechanism_name
//   function: a mechanism as the driver names it; "" for none.
const char* auth_mechanism_name(
    mongo_auth_mechanism _mechanism
) noexcept
{
    switch (_mechanism)
    {
        case mongo_auth_mechanism::scram_sha_1:   return "SCRAM-SHA-1";
        case mongo_auth_mechanism::scram_sha_256: return "SCRAM-SHA-256";
        case mongo_auth_mechanism::x509:          return "MONGODB-X509";
        case mongo_auth_mechanism::ldap:          return "PLAIN";
        case mongo_auth_mechanism::kerberos:      return "GSSAPI";
        case mongo_auth_mechanism::oidc:          return "MONGODB-OIDC";
        case mongo_auth_mechanism::aws:           return "MONGODB-AWS";
        case mongo_auth_mechanism::none:
        default:                                  return "";
    }
}

// read_concern_name, read_preference_name
//   function: a level, or a mode, as MongoDB spells it.
const char* read_concern_name(
    mongo_read_concern _level
) noexcept
{
    switch (_level)
    {
        case mongo_read_concern::available:    return "available";
        case mongo_read_concern::majority:     return "majority";
        case mongo_read_concern::linearizable: return "linearizable";
        case mongo_read_concern::snapshot:     return "snapshot";
        case mongo_read_concern::local:
        default:                               return "local";
    }
}

const char* read_preference_name(
    mongo_read_preference _mode
) noexcept
{
    switch (_mode)
    {
        case mongo_read_preference::primary_preferred:
            return "primaryPreferred";

        case mongo_read_preference::secondary:
            return "secondary";

        case mongo_read_preference::secondary_preferred:
            return "secondaryPreferred";

        case mongo_read_preference::nearest:
            return "nearest";

        case mongo_read_preference::primary:
        default:
            return "primary";
    }
}

/**
 * @brief Fills the options of a connect: where, as whom, in what database.
 *
 * @note A connection string says where, and base's host and port are then
 *       not read; without one, they are. The options point into
 *       `_config`, which must outlive them.
 *
 * @param[in]  _config   the configuration.
 * @param[out] _options  the options.
 * @throws connection_exception with D_DB_INVALID_ARGUMENT for text that
 *         holds a NUL.
 */
void fill_where(
    const mongo_connect_config& _config,
    struct dj_mongodb_options&  _options
)
{
    const connection_config& base = _config.base;

    internal::refuse_nul_setting(_config.connection_uri,
                                 "MongoDB connect",
                                 "the connection string");
    internal::refuse_nul_setting(_config.auth_source,
                                 "MongoDB connect",
                                 "the authentication database");
    internal::refuse_nul_setting(_config.replica_set,
                                 "MongoDB connect",
                                 "the replica set");

    // where: a connection string, or a host and a port
    if (!_config.connection_uri.empty())
    {
        _options.uri = _config.connection_uri.c_str();
    }
    else
    {
        _options.host = (base.host.empty()) ? nullptr : base.host.c_str();
        _options.port = base.port;
    }

    // as whom, in what database
    _options.user        = (base.username.empty()) ? nullptr
                                                   : base.username.c_str();
    _options.password    = (base.password.empty()) ? nullptr
                                                   : base.password.c_str();
    _options.database    = (base.database.empty()) ? nullptr
                                                   : base.database.c_str();
    _options.auth_source = (_config.auth_source.empty())
                               ? nullptr
                               : _config.auth_source.c_str();
    _options.replica_set = (_config.replica_set.empty())
                               ? nullptr
                               : _config.replica_set.c_str();

    return;
}

/**
 * @brief Adds the generic configuration's settings: the timeouts and TLS.
 *
 * @param[in]     _base      the generic configuration.
 * @param[in,out] _settings  the settings.
 * @throws connection_exception with D_DB_UNSUPPORTED for a key apart from
 *         the certificate, which MongoDB reads from the certificate's file;
 *         with D_DB_INVALID_ARGUMENT for text that holds a NUL.
 */
void fill_base_settings(
    const connection_config& _base,
    connect_settings&        _settings
)
{
    // the socket's wait, and the reply's
    if (_base.connect_timeout.count() > 0)
    {
        _settings.number(D_MONGODB_SETTING_CONNECT_TIMEOUT,
                         static_cast<unsigned long>(
                             _base.connect_timeout.count()));
    }

    if (_base.read_timeout.count() > 0)
    {
        _settings.number(D_MONGODB_SETTING_SOCKET_TIMEOUT,
                         static_cast<unsigned long>(
                             _base.read_timeout.count()));
    }

    // a key MongoDB cannot take apart from its certificate
    if ( (_base.ssl_key.has_value())  &&
         (!_base.ssl_key->empty())    &&
         (_base.ssl_key != _base.ssl_cert) )
    {
        throw connection_exception("MongoDB connect: the client's key is "
                                   "read from its certificate's file, "
                                   "ssl_cert, and never apart",
                                   D_DB_UNSUPPORTED);
    }

    // TLS, its authorities, the client's certificate, and the checks
    if (_base.enable_ssl)
    {
        _settings.number(D_MONGODB_SETTING_TLS, 1ul);

        if (!_base.verify_ssl)
        {
            _settings.number(
                D_MONGODB_SETTING_TLS_ALLOW_INVALID_CERTIFICATES, 1ul);
            _settings.number(D_MONGODB_SETTING_TLS_ALLOW_INVALID_HOSTNAMES,
                             1ul);
        }
    }

    if ( (_base.ssl_ca.has_value()) &&
         (!_base.ssl_ca->empty()) )
    {
        _settings.text(D_MONGODB_SETTING_TLS_CA_FILE, *_base.ssl_ca);
    }

    if ( (_base.ssl_cert.has_value()) &&
         (!_base.ssl_cert->empty()) )
    {
        _settings.text(D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_FILE,
                       *_base.ssl_cert);
    }

    return;
}

/**
 * @brief Adds MongoDB's own settings, those the layer has a name for.
 *
 * @param[in]     _config    the configuration.
 * @param[in,out] _settings  the settings.
 * @throws connection_exception with D_DB_INVALID_ARGUMENT for text that
 *         holds a NUL.
 */
void fill_named_settings(
    const mongo_connect_config& _config,
    connect_settings&           _settings
)
{
    // the switches and names the layer has settings for
    if (_config.server_selection_timeout_ms.has_value())
    {
        _settings.number(D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT,
                         static_cast<unsigned long>(
                             *_config.server_selection_timeout_ms));
    }

    if (!_config.app_name.empty())
    {
        _settings.text(D_MONGODB_SETTING_APP_NAME, _config.app_name);
    }

    if (_config.auth_mechanism != mongo_auth_mechanism::none)
    {
        _settings.text(D_MONGODB_SETTING_AUTH_MECHANISM,
                       auth_mechanism_name(_config.auth_mechanism));
    }

    if (!_config.compressors.empty())
    {
        _settings.text(D_MONGODB_SETTING_COMPRESSORS, _config.compressors);
    }

    if (_config.retry_writes.has_value())
    {
        _settings.number(D_MONGODB_SETTING_RETRY_WRITES,
                         (*_config.retry_writes) ? 1ul : 0ul);
    }

    if (_config.retry_reads.has_value())
    {
        _settings.number(D_MONGODB_SETTING_RETRY_READS,
                         (*_config.retry_reads) ? 1ul : 0ul);
    }

    if (_config.direct_connection)
    {
        _settings.number(D_MONGODB_SETTING_DIRECT_CONNECTION, 1ul);
    }

    if (_config.load_balanced)
    {
        _settings.number(D_MONGODB_SETTING_LOAD_BALANCED, 1ul);
    }

    if (!_config.stable_api_version.empty())
    {
        _settings.text(D_MONGODB_SETTING_SERVER_API,
                       _config.stable_api_version,
                       (_config.stable_api_strict) ? D_MONGODB_API_STRICT
                                                   : 0ul);
    }

    return;
}

/**
 * @brief Adds the settings a connection string's options carry: the
 *        concerns, the preference, and the options by name.
 *
 * @param[in]     _config    the configuration.
 * @param[in,out] _settings  the settings.
 * @throws connection_exception with D_DB_INVALID_ARGUMENT for text that
 *         holds a NUL.
 */
void fill_option_settings(
    const mongo_connect_config& _config,
    connect_settings&           _settings
)
{
    // the concerns and the preference
    if (_config.read_concern.has_value())
    {
        _settings.option("readConcernLevel",
                         read_concern_name(*_config.read_concern));
    }

    if (_config.write_concern.has_value())
    {
        _settings.option("w",
                         (*_config.write_concern ==
                          mongo_write_concern::majority) ? "majority"
                                                         : "1");
    }

    if (_config.read_preference.has_value())
    {
        _settings.option("readPreference",
                         read_preference_name(*_config.read_preference));
    }

    // the options by name, the generic configuration's first
    for (const auto& option : _config.base.custom_options)
    {
        _settings.option(option.first, option.second);
    }

    for (const auto& option : _config.extra_options)
    {
        _settings.option(option.first, option.second);
    }

    return;
}

/**
 * @brief Reads an _id that is an integer, as Extended JSON writes one: a
 *        number, or {"$numberInt": "..."} or {"$numberLong": "..."}.
 *
 * @param[in]  _json  the _id.
 * @param[out] _id    receives the integer; written only on success.
 * @return whether the _id is an integer.
 */
bool integer_id(
    const std::string& _json,
    re_std::int64_t&   _id
)
{
    std::string compact;

    // the text without its spaces, which no integer's form holds within
    for (const char c : _json)
    {
        if ( (c != ' ')  &&
             (c != '\t') &&
             (c != '\n') &&
             (c != '\r') )
        {
            compact += c;
        }
    }

    std::string digits = compact;

    // a canonical number's digits, between its quotes
    for (const char* form : { "{\"$numberInt\":\"", "{\"$numberLong\":\"" })
    {
        const std::string prefix(form);

        if ( (compact.size() > prefix.size() + 2u)              &&
             (compact.compare(0u, prefix.size(), prefix) == 0) &&
             (compact.compare(compact.size() - 2u, 2u, "\"}") == 0) )
        {
            digits = compact.substr(prefix.size(),
                                    compact.size() - prefix.size() - 2u);
        }
    }

    std::size_t used   = 0u;
    long long   parsed = 0;
    const bool  sign   = ( (!digits.empty()) &&
                           (digits[0] == '-') );

    // digits alone, every one read, within int64_t
    if ( (digits.size() <= ((sign) ? 1u : 0u))                         ||
         (digits.find_first_not_of("0123456789", (sign) ? 1u : 0u) !=
          std::string::npos) )
    {
        return false;
    }

    try
    {
        parsed = std::stoll(digits, &used);
    }
    catch (const std::exception&)
    {
        return false;
    }

    _id = static_cast<re_std::int64_t>(parsed);

    return (used == digits.size());
}

/**
 * @brief A layer's counts as the core's.
 *
 * @param[in] _counts  the layer's.
 * @return the counts.
 */
mongo_write_result result_of(
    const struct dj_mongodb_write_counts& _counts
) noexcept
{
    mongo_write_result result;

    result.inserted = _counts.inserted;
    result.matched  = _counts.matched;
    result.modified = _counts.modified;
    result.deleted  = _counts.deleted;
    result.upserted = _counts.upserted;

    return result;
}

/**
 * @brief Raises a failure of a cursor's or a stream's read, the session
 *        marked lost where it lost the connection; a success returns.
 *
 * @param[in] _session    the connection's session.
 * @param[in] _db         the handle the failure was recorded on.
 * @param[in] _result     the read's result.
 * @param[in] _operation  the operation's name.
 * @throws as mongodb_layer::raise, unless `_result` is 0.
 */
void check_read(
    internal::session&   _session,
    struct ::dj_mongodb* _db,
    int                  _result,
    const char*          _operation
)
{
    // success needs nothing
    if (_result == 0)
    {
        return;
    }

    // a dropped connection is the connection's to hear of too
    if (mongodb_layer::connection_lost(_result))
    {
        _session.mark_lost();
    }

    mongodb_layer::raise(_db, failure_kind::query, _operation, _result);
}

// refuse_closed -- query_exception for a read of a closed cursor or
// stream
[[noreturn]]
void refuse_closed(
    const char* _operation,
    const char* _what
)
{
    throw query_exception(std::string("MongoDB ") + _operation + ": the " +
                              _what + " is closed",
                          D_DB_INVALID_ARGUMENT);
}

}  // namespace


//==============================================================================
// 6.  MONGODB CONNECTION
//==============================================================================

// 6.2    The layer
//------------------------------------------------------------------------------
/*
mongodb_layer::describe
  The handle's message is about its latest failure, recorded with the result
the failing call returned; where the handle holds another result, or there
is none, the result's own description stands in.
*/
std::string mongodb_layer::describe(
    handle_type _db,
    const char* _operation,
    int         _result
)
{
    const char* const said = ( (_db) &&
                               (d_mongodb_error_code(_db) == _result) )
                                 ? d_mongodb_error_message(_db)
                                 : nullptr;
    const char* const line = ( (said) &&
                               (said[0] != '\0') )
                                 ? said
                                 : d_mongodb_result_message(_result);

    return std::string("MongoDB ") + _operation + ": " + line + " (" +
           code_text(_db, _result) + ")";
}

void mongodb_layer::raise_as(
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

void mongodb_layer::raise(
    handle_type  _db,
    failure_kind _kind,
    const char*  _operation,
    int          _result
)
{
    raise_as(_kind, describe(_db, _operation, _result), _result);
}

// 6.3    Cursors and change streams
//------------------------------------------------------------------------------

/*
mongo_cursor::mongo_cursor
  A cursor the session cannot enroll goes as it came: the member's
destructor closes it.
*/
mongo_cursor::mongo_cursor(
    std::shared_ptr<internal::session> _session,
    struct ::dj_mongodb*                _db,
    face::mongodb_cursor&&             _cursor
)
    : m_session(std::move(_session)),
      m_db(_db),
      m_cursor(std::move(_cursor))
{
    m_session->enroll(this, &mongo_cursor::m_closed);
}

mongo_cursor::~mongo_cursor()
{
    close();
}

std::unique_ptr<buffered_result_set> mongo_cursor::fetch(
    std::size_t _max
)
{
    // a cursor the program or the connection closed
    if (!m_cursor.is_open())
    {
        refuse_closed("fetch", "cursor");
    }

    face::rows page;

    check_read(*m_session, m_db, m_cursor.fetch(_max, page), "fetch");

    return std::make_unique<buffered_result_set>(std::move(page));
}

bool mongo_cursor::is_open() const noexcept
{
    return m_cursor.is_open();
}

bool mongo_cursor::is_done() const noexcept
{
    return m_cursor.is_done();
}

void mongo_cursor::close() noexcept
{
    // still the session's, until now
    if (m_cursor.is_open())
    {
        m_session->withdraw(this);
        m_cursor.close();
    }

    return;
}

/*
mongo_cursor::m_closed
  The session has forgotten the cursor already; only the driver's is left.
*/
void mongo_cursor::m_closed(
    void* _cursor
) noexcept
{
    static_cast<mongo_cursor*>(_cursor)->m_cursor.close();

    return;
}

mongo_change_stream::mongo_change_stream(
    std::shared_ptr<internal::session> _session,
    struct ::dj_mongodb*                _db,
    face::mongodb_change_stream&&      _stream
)
    : m_session(std::move(_session)),
      m_db(_db),
      m_stream(std::move(_stream))
{
    m_session->enroll(this, &mongo_change_stream::m_closed);
}

mongo_change_stream::~mongo_change_stream()
{
    close();
}

std::unique_ptr<buffered_result_set> mongo_change_stream::fetch(
    std::size_t _max
)
{
    // a stream the program or the connection closed
    if (!m_stream.is_open())
    {
        refuse_closed("fetch", "change stream");
    }

    face::rows events;

    check_read(*m_session, m_db, m_stream.fetch(_max, events), "fetch");

    return std::make_unique<buffered_result_set>(std::move(events));
}

/*
mongo_change_stream::resume_token
  A stream that has seen no batch has no token yet: nothing, not a failure.
*/
std::optional<std::string> mongo_change_stream::resume_token() const
{
    // a stream the program or the connection closed
    if (!m_stream.is_open())
    {
        refuse_closed("resume_token", "change stream");
    }

    std::string token;
    const int   result = m_stream.resume_token(token);

    // no token yet
    if (result == D_DB_NOT_FOUND)
    {
        return std::nullopt;
    }

    check_read(*m_session, m_db, result, "resume_token");

    return token;
}

bool mongo_change_stream::is_open() const noexcept
{
    return m_stream.is_open();
}

void mongo_change_stream::close() noexcept
{
    // still the session's, until now
    if (m_stream.is_open())
    {
        m_session->withdraw(this);
        m_stream.close();
    }

    return;
}

void mongo_change_stream::m_closed(
    void* _stream
) noexcept
{
    static_cast<mongo_change_stream*>(_stream)->m_stream.close();

    return;
}

// 6.4    The class
//------------------------------------------------------------------------------
mongo_connection::mongo_connection()
    : base_type(mongo_connect_config().base),
      m_mongo_config(),
      m_db(),
      m_adopting()
{}

mongo_connection::mongo_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_mongo_config(_config),
      m_db(),
      m_adopting()
{}

mongo_connection::mongo_connection(
    const mongo_connect_config& _config
)
    : base_type(_config.base),
      m_mongo_config(_config),
      m_db(),
      m_adopting()
{}

mongo_connection::mongo_connection(
    const std::string& _uri
)
    : base_type(mongo_connect_config(_uri).base),
      m_mongo_config(_uri),
      m_db(),
      m_adopting()
{}

/*
mongo_connection::~mongo_connection
  The cursors and the change streams go first, closed on the handle they
read through while it is open; the face then closes the handle as it goes.
*/
mongo_connection::~mongo_connection()
{
    this->close_session_noexcept();
}

/*
mongo_connection::adopt
  connect() does the rest, as for any connection, connect_impl taking the
client in place of the configuration's; an open connection is closed
first, since connect() leaves one as it is.
*/
void mongo_connection::adopt(
    void* _native,
    bool  _owned
)
{
    this->disconnect();
    m_adopting = adoption{ _native, _owned };

    try
    {
        this->connect();
    }
    catch (...)
    {
        m_adopting.reset();

        throw;
    }

    m_adopting.reset();

    return;
}

std::string mongo_connection::run_command(
    const std::string& _command,
    const std::string& _database
)
{
    return this->run_guarded(
        [this, &_command, &_database]() -> std::string
        {
            std::string reply;

            internal::refuse_nul(_command, "run_command", "the command");
            internal::refuse_nul(_database, "run_command", "the database");
            m_check(m_db.command(optional_arg(_database), reply, _command),
                    failure_kind::query,
                    "run_command");
            m_wrote();

            return reply;
        });
}

// 6.4.1.1  Documents
std::string mongo_connection::insert_one(
    const std::string& _collection,
    const std::string& _document
)
{
    return this->run_guarded(
        [this, &_collection, &_document]() -> std::string
        {
            std::string id;

            internal::refuse_nul(_collection, "insert_one", "the collection");
            internal::refuse_nul(_document, "insert_one", "the document");
            m_check(m_db.insert_one(face::string_arg(),
                                    _collection,
                                    _document,
                                    id),
                    failure_kind::query,
                    "insert_one");
            m_wrote();

            return id;
        });
}

/*
mongo_connection::insert_many
  What went in before a failure is not undone; the count of it goes with
the failure, recorded as what changed.
*/
re_std::int64_t mongo_connection::insert_many(
    const std::string& _collection,
    const std::string& _documents,
    bool               _ordered
)
{
    return this->run_guarded(
        [this, &_collection, &_documents, _ordered]() -> re_std::int64_t
        {
            re_std::int64_t inserted = 0;

            internal::refuse_nul(_collection, "insert_many", "the collection");
            internal::refuse_nul(_documents, "insert_many", "the documents");

            const int result = m_db.insert_many(
                face::string_arg(),
                _collection,
                _documents,
                (_ordered) ? 0u : static_cast<unsigned>(D_MONGODB_UNORDERED),
                inserted);

            this->record_changes(inserted);
            m_check(result, failure_kind::query, "insert_many");

            return inserted;
        });
}

std::unique_ptr<buffered_result_set> mongo_connection::find(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]()
            -> std::unique_ptr<buffered_result_set>
        {
            face::rows found;

            internal::refuse_nul(_collection, "find", "the collection");
            internal::refuse_nul(_filter, "find", "the filter");
            internal::refuse_nul(_options, "find", "the options");
            m_check(m_db.find(face::string_arg(),
                              _collection,
                              optional_arg(_filter),
                              optional_arg(_options),
                              found),
                    failure_kind::query,
                    "find");

            return m_rows(std::move(found));
        });
}

/*
mongo_connection::find_one
  None matched is no document, not a failure.
*/
std::optional<std::string> mongo_connection::find_one(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]()
            -> std::optional<std::string>
        {
            face::rows found;

            internal::refuse_nul(_collection, "find_one", "the collection");
            internal::refuse_nul(_filter, "find_one", "the filter");
            internal::refuse_nul(_options, "find_one", "the options");

            const int result = m_db.find_one(face::string_arg(),
                                             _collection,
                                             optional_arg(_filter),
                                             optional_arg(_options),
                                             found);

            // none matched
            if (result == D_DB_NOT_FOUND)
            {
                return std::nullopt;
            }

            m_check(result, failure_kind::query, "find_one");

            return m_document(found);
        });
}

std::unique_ptr<buffered_result_set> mongo_connection::find_keyed(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]()
            -> std::unique_ptr<buffered_result_set>
        {
            face::rows found;

            internal::refuse_nul(_collection, "find_keyed", "the collection");
            internal::refuse_nul(_filter, "find_keyed", "the filter");
            internal::refuse_nul(_options, "find_keyed", "the options");
            m_check(m_db.find_keyed(face::string_arg(),
                                    _collection,
                                    optional_arg(_filter),
                                    optional_arg(_options),
                                    found),
                    failure_kind::query,
                    "find_keyed");

            return m_rows(std::move(found));
        });
}

mongo_write_result mongo_connection::update_one(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _update,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_update, &_options]()
            -> mongo_write_result
        {
            struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

            internal::refuse_nul(_collection, "update_one", "the collection");
            internal::refuse_nul(_filter, "update_one", "the filter");
            internal::refuse_nul(_update, "update_one", "the update");
            internal::refuse_nul(_options, "update_one", "the options");
            m_check(m_db.update_one(face::string_arg(),
                                    _collection,
                                    optional_arg(_filter),
                                    _update,
                                    optional_arg(_options),
                                    counts),
                    failure_kind::query,
                    "update_one");
            m_wrote();

            return result_of(counts);
        });
}

mongo_write_result mongo_connection::update_many(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _update,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_update, &_options]()
            -> mongo_write_result
        {
            struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

            internal::refuse_nul(_collection, "update_many", "the collection");
            internal::refuse_nul(_filter, "update_many", "the filter");
            internal::refuse_nul(_update, "update_many", "the update");
            internal::refuse_nul(_options, "update_many", "the options");
            m_check(m_db.update_many(face::string_arg(),
                                     _collection,
                                     optional_arg(_filter),
                                     _update,
                                     optional_arg(_options),
                                     counts),
                    failure_kind::query,
                    "update_many");
            m_wrote();

            return result_of(counts);
        });
}

mongo_write_result mongo_connection::replace_one(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _replacement,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_replacement, &_options]()
            -> mongo_write_result
        {
            struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

            internal::refuse_nul(_collection, "replace_one", "the collection");
            internal::refuse_nul(_filter, "replace_one", "the filter");
            internal::refuse_nul(_replacement,
                                 "replace_one",
                                 "the replacement");
            internal::refuse_nul(_options, "replace_one", "the options");
            m_check(m_db.replace_one(face::string_arg(),
                                     _collection,
                                     optional_arg(_filter),
                                     _replacement,
                                     optional_arg(_options),
                                     counts),
                    failure_kind::query,
                    "replace_one");
            m_wrote();

            return result_of(counts);
        });
}

re_std::int64_t mongo_connection::delete_one(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]() -> re_std::int64_t
        {
            re_std::int64_t deleted = 0;

            internal::refuse_nul(_collection, "delete_one", "the collection");
            internal::refuse_nul(_filter, "delete_one", "the filter");
            internal::refuse_nul(_options, "delete_one", "the options");
            m_check(m_db.delete_one(face::string_arg(),
                                    _collection,
                                    optional_arg(_filter),
                                    optional_arg(_options),
                                    deleted),
                    failure_kind::query,
                    "delete_one");
            m_wrote();

            return deleted;
        });
}

re_std::int64_t mongo_connection::delete_many(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]() -> re_std::int64_t
        {
            re_std::int64_t deleted = 0;

            internal::refuse_nul(_collection, "delete_many", "the collection");
            internal::refuse_nul(_filter, "delete_many", "the filter");
            internal::refuse_nul(_options, "delete_many", "the options");
            m_check(m_db.delete_many(face::string_arg(),
                                     _collection,
                                     optional_arg(_filter),
                                     optional_arg(_options),
                                     deleted),
                    failure_kind::query,
                    "delete_many");
            m_wrote();

            return deleted;
        });
}

re_std::int64_t mongo_connection::count_documents(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
) const
{
    return this->ask_guarded(
        [this, &_collection, &_filter, &_options]() -> re_std::int64_t
        {
            re_std::int64_t count = 0;

            internal::refuse_nul(_collection,
                                 "count_documents",
                                 "the collection");
            internal::refuse_nul(_filter, "count_documents", "the filter");
            internal::refuse_nul(_options, "count_documents", "the options");
            m_check(m_db.count_documents(face::string_arg(),
                                         _collection,
                                         optional_arg(_filter),
                                         optional_arg(_options),
                                         count),
                    failure_kind::query,
                    "count_documents");

            return count;
        });
}

re_std::int64_t mongo_connection::estimated_document_count(
    const std::string& _collection,
    const std::string& _options
) const
{
    return this->ask_guarded(
        [this, &_collection, &_options]() -> re_std::int64_t
        {
            re_std::int64_t count = 0;

            internal::refuse_nul(_collection,
                                 "estimated_document_count",
                                 "the collection");
            internal::refuse_nul(_options,
                                 "estimated_document_count",
                                 "the options");
            m_check(m_db.estimated_document_count(face::string_arg(),
                                                  _collection,
                                                  optional_arg(_options),
                                                  count),
                    failure_kind::query,
                    "estimated_document_count");

            return count;
        });
}

std::unique_ptr<buffered_result_set> mongo_connection::distinct(
    const std::string& _collection,
    const std::string& _field,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_field, &_filter, &_options]()
            -> std::unique_ptr<buffered_result_set>
        {
            face::rows values;

            internal::refuse_nul(_collection, "distinct", "the collection");
            internal::refuse_nul(_field, "distinct", "the field");
            internal::refuse_nul(_filter, "distinct", "the filter");
            internal::refuse_nul(_options, "distinct", "the options");
            m_check(m_db.distinct(face::string_arg(),
                                  _collection,
                                  _field,
                                  optional_arg(_filter),
                                  optional_arg(_options),
                                  values),
                    failure_kind::query,
                    "distinct");

            return m_rows(std::move(values));
        });
}

/*
mongo_connection::aggregate
  A collection of "" is the layer's NULL: the database's own stages.
*/
std::unique_ptr<buffered_result_set> mongo_connection::aggregate(
    const std::string& _collection,
    const std::string& _pipeline,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_pipeline, &_options]()
            -> std::unique_ptr<buffered_result_set>
        {
            face::rows found;

            internal::refuse_nul(_collection, "aggregate", "the collection");
            internal::refuse_nul(_pipeline, "aggregate", "the pipeline");
            internal::refuse_nul(_options, "aggregate", "the options");
            m_check(m_db.aggregate(face::string_arg(),
                                   optional_arg(_collection),
                                   _pipeline,
                                   optional_arg(_options),
                                   found),
                    failure_kind::query,
                    "aggregate");

            return m_rows(std::move(found));
        });
}

/*
mongo_connection::find_one_and_update
  No document is the layer's D_DB_NOT_FOUND, the write made all the same,
so what changed is recorded either way.
*/
std::optional<std::string> mongo_connection::find_one_and_update(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _update,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_update, &_options]()
            -> std::optional<std::string>
        {
            face::rows found;

            internal::refuse_nul(_collection,
                                 "find_one_and_update",
                                 "the collection");
            internal::refuse_nul(_filter, "find_one_and_update", "the filter");
            internal::refuse_nul(_update, "find_one_and_update", "the update");
            internal::refuse_nul(_options,
                                 "find_one_and_update",
                                 "the options");

            const int result = m_db.find_one_and_update(
                face::string_arg(),
                _collection,
                optional_arg(_filter),
                _update,
                optional_arg(_options),
                found);

            return m_modified(result, found, "find_one_and_update");
        });
}

std::optional<std::string> mongo_connection::find_one_and_replace(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _replacement,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_replacement, &_options]()
            -> std::optional<std::string>
        {
            face::rows found;

            internal::refuse_nul(_collection,
                                 "find_one_and_replace",
                                 "the collection");
            internal::refuse_nul(_filter,
                                 "find_one_and_replace",
                                 "the filter");
            internal::refuse_nul(_replacement,
                                 "find_one_and_replace",
                                 "the replacement");
            internal::refuse_nul(_options,
                                 "find_one_and_replace",
                                 "the options");

            const int result = m_db.find_one_and_replace(
                face::string_arg(),
                _collection,
                optional_arg(_filter),
                _replacement,
                optional_arg(_options),
                found);

            return m_modified(result, found, "find_one_and_replace");
        });
}

std::optional<std::string> mongo_connection::find_one_and_delete(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]()
            -> std::optional<std::string>
        {
            face::rows found;

            internal::refuse_nul(_collection,
                                 "find_one_and_delete",
                                 "the collection");
            internal::refuse_nul(_filter, "find_one_and_delete", "the filter");
            internal::refuse_nul(_options,
                                 "find_one_and_delete",
                                 "the options");

            const int result = m_db.find_one_and_delete(
                face::string_arg(),
                _collection,
                optional_arg(_filter),
                optional_arg(_options),
                found);

            return m_modified(result, found, "find_one_and_delete");
        });
}

/*
mongo_connection::bulk_write
  The layer counts what went in before a failure, which is not undone; the
count is recorded as what changed, the failure then thrown.
*/
mongo_write_result mongo_connection::bulk_write(
    const std::string& _collection,
    const std::string& _operations,
    bool               _ordered,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_operations, _ordered, &_options]()
            -> mongo_write_result
        {
            struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

            internal::refuse_nul(_collection, "bulk_write", "the collection");
            internal::refuse_nul(_operations, "bulk_write", "the operations");
            internal::refuse_nul(_options, "bulk_write", "the options");

            const int result = m_db.bulk_write(
                face::string_arg(),
                _collection,
                _operations,
                (_ordered) ? 0u : static_cast<unsigned>(D_MONGODB_UNORDERED),
                optional_arg(_options),
                counts);

            m_wrote();
            m_check(result, failure_kind::query, "bulk_write");

            return result_of(counts);
        });
}

mongo_write_result mongo_connection::client_bulk_write(
    const std::string& _operations,
    bool               _ordered,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_operations, _ordered, &_options]() -> mongo_write_result
        {
            struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

            internal::refuse_nul(_operations,
                                 "client_bulk_write",
                                 "the operations");
            internal::refuse_nul(_options, "client_bulk_write", "the options");

            const int result = m_db.client_bulk_write(
                _operations,
                (_ordered) ? 0u : static_cast<unsigned>(D_MONGODB_UNORDERED),
                optional_arg(_options),
                counts);

            m_wrote();
            m_check(result, failure_kind::query, "client_bulk_write");

            return result_of(counts);
        });
}

// 6.4.1.2  Cursors and change streams
std::unique_ptr<mongo_cursor> mongo_connection::find_cursor(
    const std::string& _collection,
    const std::string& _filter,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_filter, &_options]()
            -> std::unique_ptr<mongo_cursor>
        {
            face::mongodb_cursor opened;

            internal::refuse_nul(_collection, "find_cursor", "the collection");
            internal::refuse_nul(_filter, "find_cursor", "the filter");
            internal::refuse_nul(_options, "find_cursor", "the options");
            m_check(m_db.find_cursor(face::string_arg(),
                                     _collection,
                                     optional_arg(_filter),
                                     optional_arg(_options),
                                     opened),
                    failure_kind::query,
                    "find_cursor");

            return m_cursor(std::move(opened));
        });
}

std::unique_ptr<mongo_cursor> mongo_connection::aggregate_cursor(
    const std::string& _collection,
    const std::string& _pipeline,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_pipeline, &_options]()
            -> std::unique_ptr<mongo_cursor>
        {
            face::mongodb_cursor opened;

            internal::refuse_nul(_collection,
                                 "aggregate_cursor",
                                 "the collection");
            internal::refuse_nul(_pipeline, "aggregate_cursor", "the pipeline");
            internal::refuse_nul(_options, "aggregate_cursor", "the options");
            m_check(m_db.aggregate_cursor(face::string_arg(),
                                          optional_arg(_collection),
                                          _pipeline,
                                          optional_arg(_options),
                                          opened),
                    failure_kind::query,
                    "aggregate_cursor");

            return m_cursor(std::move(opened));
        });
}

std::unique_ptr<mongo_cursor> mongo_connection::command_cursor(
    const std::string& _command
)
{
    return this->run_guarded(
        [this, &_command]() -> std::unique_ptr<mongo_cursor>
        {
            face::mongodb_cursor opened;

            internal::refuse_nul(_command, "command_cursor", "the command");
            m_check(m_db.command_cursor(face::string_arg(), _command, opened),
                    failure_kind::query,
                    "command_cursor");

            return m_cursor(std::move(opened));
        });
}

std::unique_ptr<mongo_change_stream> mongo_connection::watch_collection(
    const std::string& _collection,
    const std::string& _pipeline,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_collection, &_pipeline, &_options]()
            -> std::unique_ptr<mongo_change_stream>
        {
            face::mongodb_change_stream opened;

            internal::refuse_nul(_collection,
                                 "watch_collection",
                                 "the collection");
            internal::refuse_nul(_pipeline, "watch_collection", "the pipeline");
            internal::refuse_nul(_options, "watch_collection", "the options");
            m_check(m_db.watch_collection(face::string_arg(),
                                          _collection,
                                          optional_arg(_pipeline),
                                          optional_arg(_options),
                                          opened),
                    failure_kind::query,
                    "watch_collection");

            return m_stream(std::move(opened));
        });
}

std::unique_ptr<mongo_change_stream> mongo_connection::watch_database(
    const std::string& _pipeline,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_pipeline, &_options]() -> std::unique_ptr<mongo_change_stream>
        {
            face::mongodb_change_stream opened;

            internal::refuse_nul(_pipeline, "watch_database", "the pipeline");
            internal::refuse_nul(_options, "watch_database", "the options");
            m_check(m_db.watch_database(face::string_arg(),
                                        optional_arg(_pipeline),
                                        optional_arg(_options),
                                        opened),
                    failure_kind::query,
                    "watch_database");

            return m_stream(std::move(opened));
        });
}

std::unique_ptr<mongo_change_stream> mongo_connection::watch_deployment(
    const std::string& _pipeline,
    const std::string& _options
)
{
    return this->run_guarded(
        [this, &_pipeline, &_options]() -> std::unique_ptr<mongo_change_stream>
        {
            face::mongodb_change_stream opened;

            internal::refuse_nul(_pipeline, "watch_deployment", "the pipeline");
            internal::refuse_nul(_options, "watch_deployment", "the options");
            m_check(m_db.watch_deployment(optional_arg(_pipeline),
                                          optional_arg(_options),
                                          opened),
                    failure_kind::query,
                    "watch_deployment");

            return m_stream(std::move(opened));
        });
}

// 6.4.1.3  Databases, collections, views and indexes
bool mongo_connection::has_database(
    const std::string& _name
) const
{
    return this->ask_guarded(
        [this, &_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_name, "has_database", "the name");
            m_check(m_db.has_database_s(_name, exists),
                    failure_kind::query,
                    "has_database");

            return exists;
        });
}

void mongo_connection::drop_database(
    const std::string& _name,
    bool               _if_exists
)
{
    this->run_guarded(
        [this, &_name, _if_exists]() -> void
        {
            internal::refuse_nul(_name, "drop_database", "the name");
            m_check(m_db.drop_database(_name,
                                       (_if_exists) ? D_DB_IF_EXISTS : 0u),
                    failure_kind::query,
                    "drop_database");

            return;
        });

    return;
}

std::vector<std::string> mongo_connection::list_databases() const
{
    return this->ask_guarded(
        [this]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            m_check(m_db.list_databases(names),
                    failure_kind::query,
                    "list_databases");

            return names;
        });
}

/*
mongo_connection::use_database
  The layer checks the name as the server would at the first operation;
the configuration names the database from then on, for a reconnect.
*/
void mongo_connection::use_database(
    const std::string& _name
)
{
    this->run_guarded(
        [this, &_name]() -> void
        {
            internal::refuse_nul(_name, "use_database", "the name");
            m_check(m_db.use_database(_name),
                    failure_kind::query,
                    "use_database");

            return;
        });

    this->m_config.database = _name;

    return;
}

bool mongo_connection::has_collection(
    const std::string& _name
) const
{
    return this->ask_guarded(
        [this, &_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_name, "has_collection", "the name");
            m_check(m_db.has_collection_s(face::string_arg(), _name, exists),
                    failure_kind::query,
                    "has_collection");

            return exists;
        });
}

void mongo_connection::create_collection(
    const std::string& _name,
    const std::string& _options,
    bool               _if_not_exists
)
{
    this->run_guarded(
        [this, &_name, &_options, _if_not_exists]() -> void
        {
            internal::refuse_nul(_name, "create_collection", "the name");
            internal::refuse_nul(_options, "create_collection", "the options");
            m_check(m_db.create_collection(
                        face::string_arg(),
                        _name,
                        optional_arg(_options),
                        (_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u),
                    failure_kind::query,
                    "create_collection");

            return;
        });

    return;
}

void mongo_connection::drop_collection(
    const std::string& _name,
    bool               _if_exists
)
{
    this->run_guarded(
        [this, &_name, _if_exists]() -> void
        {
            internal::refuse_nul(_name, "drop_collection", "the name");
            m_check(m_db.drop_collection(face::string_arg(),
                                         _name,
                                         (_if_exists) ? D_DB_IF_EXISTS : 0u),
                    failure_kind::query,
                    "drop_collection");

            return;
        });

    return;
}

void mongo_connection::rename_collection(
    const std::string& _from,
    const std::string& _to,
    bool               _drop_target
)
{
    this->run_guarded(
        [this, &_from, &_to, _drop_target]() -> void
        {
            internal::refuse_nul(_from, "rename_collection", "the name");
            internal::refuse_nul(_to, "rename_collection", "the new name");
            m_check(m_db.rename_collection(
                        face::string_arg(),
                        _from,
                        face::string_arg(),
                        _to,
                        (_drop_target)
                            ? static_cast<unsigned>(D_MONGODB_DROP_TARGET)
                            : 0u),
                    failure_kind::query,
                    "rename_collection");

            return;
        });

    return;
}

std::vector<std::string> mongo_connection::list_collections() const
{
    return this->ask_guarded(
        [this]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            m_check(m_db.list_collections(face::string_arg(), names),
                    failure_kind::query,
                    "list_collections");

            return names;
        });
}

std::string mongo_connection::get_collection_info(
    const std::string& _name
) const
{
    return this->ask_guarded(
        [this, &_name]() -> std::string
        {
            std::string info;

            internal::refuse_nul(_name, "get_collection_info", "the name");
            m_check(m_db.collection_info(face::string_arg(), _name, info),
                    failure_kind::query,
                    "get_collection_info");

            return info;
        });
}

bool mongo_connection::has_view(
    const std::string& _name
) const
{
    return this->ask_guarded(
        [this, &_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_name, "has_view", "the name");
            m_check(m_db.has_view_s(face::string_arg(), _name, exists),
                    failure_kind::query,
                    "has_view");

            return exists;
        });
}

void mongo_connection::create_view(
    const std::string& _name,
    const std::string& _source,
    const std::string& _pipeline,
    const std::string& _options,
    bool               _if_not_exists
)
{
    this->run_guarded(
        [this, &_name, &_source, &_pipeline, &_options, _if_not_exists]()
            -> void
        {
            internal::refuse_nul(_name, "create_view", "the name");
            internal::refuse_nul(_source, "create_view", "the source");
            internal::refuse_nul(_pipeline, "create_view", "the pipeline");
            internal::refuse_nul(_options, "create_view", "the options");
            m_check(m_db.create_view(
                        face::string_arg(),
                        _name,
                        _source,
                        _pipeline,
                        optional_arg(_options),
                        (_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u),
                    failure_kind::query,
                    "create_view");

            return;
        });

    return;
}

void mongo_connection::drop_view(
    const std::string& _name,
    bool               _if_exists
)
{
    this->run_guarded(
        [this, &_name, _if_exists]() -> void
        {
            internal::refuse_nul(_name, "drop_view", "the name");
            m_check(m_db.drop_view(face::string_arg(),
                                   _name,
                                   (_if_exists) ? D_DB_IF_EXISTS : 0u),
                    failure_kind::query,
                    "drop_view");

            return;
        });

    return;
}

std::vector<std::string> mongo_connection::list_views() const
{
    return this->ask_guarded(
        [this]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            m_check(m_db.list_views(face::string_arg(), names),
                    failure_kind::query,
                    "list_views");

            return names;
        });
}

bool mongo_connection::has_index(
    const std::string& _collection,
    const std::string& _name
) const
{
    return this->ask_guarded(
        [this, &_collection, &_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_collection, "has_index", "the collection");
            internal::refuse_nul(_name, "has_index", "the name");
            m_check(m_db.has_index_s(face::string_arg(),
                                     _collection,
                                     _name,
                                     exists),
                    failure_kind::query,
                    "has_index");

            return exists;
        });
}

void mongo_connection::create_index(
    const std::string& _collection,
    const std::string& _name,
    const std::string& _keys,
    bool               _unique,
    const std::string& _options,
    bool               _if_not_exists
)
{
    const unsigned flags = ( (_unique) ? D_DB_UNIQUE : 0u ) |
                           ( (_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u );

    this->run_guarded(
        [this, &_collection, &_name, &_keys, &_options, flags]() -> void
        {
            internal::refuse_nul(_collection,
                                 "create_index",
                                 "the collection");
            internal::refuse_nul(_name, "create_index", "the name");
            internal::refuse_nul(_keys, "create_index", "the keys");
            internal::refuse_nul(_options, "create_index", "the options");
            m_check(m_db.create_index(face::string_arg(),
                                      _collection,
                                      _name,
                                      _keys,
                                      optional_arg(_options),
                                      flags),
                    failure_kind::query,
                    "create_index");

            return;
        });

    return;
}

void mongo_connection::drop_index(
    const std::string& _collection,
    const std::string& _name,
    bool               _if_exists
)
{
    this->run_guarded(
        [this, &_collection, &_name, _if_exists]() -> void
        {
            internal::refuse_nul(_collection, "drop_index", "the collection");
            internal::refuse_nul(_name, "drop_index", "the name");
            m_check(m_db.drop_index(face::string_arg(),
                                    _collection,
                                    _name,
                                    (_if_exists) ? D_DB_IF_EXISTS : 0u),
                    failure_kind::query,
                    "drop_index");

            return;
        });

    return;
}

std::vector<std::string> mongo_connection::list_indexes(
    const std::string& _collection
) const
{
    return this->ask_guarded(
        [this, &_collection]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            internal::refuse_nul(_collection,
                                 "list_indexes",
                                 "the collection");
            m_check(m_db.list_indexes(face::string_arg(), _collection, names),
                    failure_kind::query,
                    "list_indexes");

            return names;
        });
}

std::unique_ptr<buffered_result_set> mongo_connection::describe_indexes(
    const std::string& _collection
) const
{
    return this->ask_guarded(
        [this, &_collection]() -> std::unique_ptr<buffered_result_set>
        {
            face::rows described;

            internal::refuse_nul(_collection,
                                 "describe_indexes",
                                 "the collection");
            m_check(m_db.describe_indexes(face::string_arg(),
                                          _collection,
                                          described),
                    failure_kind::query,
                    "describe_indexes");

            return m_rows(std::move(described));
        });
}

// 6.4.1.4  Transactions
void mongo_connection::begin_transaction(
    const std::string& _options
)
{
    this->run_guarded(
        [this, &_options]() -> void
        {
            internal::refuse_nul(_options, "begin_transaction", "the options");
            m_check(m_db.begin_transaction_with(optional_arg(_options)),
                    failure_kind::transaction,
                    "begin_transaction");

            return;
        });

    return;
}

bool mongo_connection::in_failed_transaction() const noexcept
{
    return m_db.in_failed_transaction();
}

// 6.4.1.5  GridFS
std::string mongo_connection::gridfs_upload(
    const std::string&                  _filename,
    const std::vector<re_std::uint8_t>& _data,
    const std::string&                  _metadata,
    const std::string&                  _bucket
)
{
    return this->run_guarded(
        [this, &_filename, &_data, &_metadata, &_bucket]() -> std::string
        {
            std::string id;

            internal::refuse_nul(_filename, "gridfs_upload", "the file name");
            internal::refuse_nul(_metadata, "gridfs_upload", "the metadata");
            internal::refuse_nul(_bucket, "gridfs_upload", "the bucket");
            m_check(m_db.gridfs_upload(face::string_arg(),
                                       optional_arg(_bucket),
                                       _filename,
                                       (_data.empty()) ? nullptr
                                                       : _data.data(),
                                       _data.size(),
                                       optional_arg(_metadata),
                                       id),
                    failure_kind::query,
                    "gridfs_upload");

            return id;
        });
}

std::string mongo_connection::gridfs_upload_file(
    const std::string& _filename,
    const std::string& _path,
    const std::string& _metadata,
    const std::string& _bucket
)
{
    return this->run_guarded(
        [this, &_filename, &_path, &_metadata, &_bucket]() -> std::string
        {
            std::string id;

            internal::refuse_nul(_filename,
                                 "gridfs_upload_file",
                                 "the file name");
            internal::refuse_nul(_path, "gridfs_upload_file", "the path");
            internal::refuse_nul(_metadata,
                                 "gridfs_upload_file",
                                 "the metadata");
            internal::refuse_nul(_bucket, "gridfs_upload_file", "the bucket");
            m_check(m_db.gridfs_upload_file(face::string_arg(),
                                            optional_arg(_bucket),
                                            _filename,
                                            _path,
                                            optional_arg(_metadata),
                                            id),
                    failure_kind::query,
                    "gridfs_upload_file");

            return id;
        });
}

/*
mongo_connection::gridfs_download
  The layer's bytes are copied out and released at once; a std::vector
that cannot grow leaves them released all the same.
*/
std::vector<re_std::uint8_t> mongo_connection::gridfs_download(
    const std::string& _id,
    const std::string& _bucket
)
{
    return this->run_guarded(
        [this, &_id, &_bucket]() -> std::vector<re_std::uint8_t>
        {
            void*       data = nullptr;
            std::size_t size = 0u;

            internal::refuse_nul(_id, "gridfs_download", "the _id");
            internal::refuse_nul(_bucket, "gridfs_download", "the bucket");
            m_check(m_db.gridfs_download(face::string_arg(),
                                         optional_arg(_bucket),
                                         _id,
                                         data,
                                         size),
                    failure_kind::query,
                    "gridfs_download");

            std::unique_ptr<void, void (*)(void*)> held(data, &d_db_free);
            const re_std::uint8_t* const           bytes =
                static_cast<const re_std::uint8_t*>(held.get());

            return (size == 0u)
                       ? std::vector<re_std::uint8_t>()
                       : std::vector<re_std::uint8_t>(bytes, bytes + size);
        });
}

void mongo_connection::gridfs_download_file(
    const std::string& _id,
    const std::string& _path,
    const std::string& _bucket
)
{
    this->run_guarded(
        [this, &_id, &_path, &_bucket]() -> void
        {
            internal::refuse_nul(_id, "gridfs_download_file", "the _id");
            internal::refuse_nul(_path, "gridfs_download_file", "the path");
            internal::refuse_nul(_bucket,
                                 "gridfs_download_file",
                                 "the bucket");
            m_check(m_db.gridfs_download_file(face::string_arg(),
                                              optional_arg(_bucket),
                                              _id,
                                              _path),
                    failure_kind::query,
                    "gridfs_download_file");

            return;
        });

    return;
}

void mongo_connection::gridfs_delete(
    const std::string& _id,
    const std::string& _bucket
)
{
    this->run_guarded(
        [this, &_id, &_bucket]() -> void
        {
            internal::refuse_nul(_id, "gridfs_delete", "the _id");
            internal::refuse_nul(_bucket, "gridfs_delete", "the bucket");
            m_check(m_db.gridfs_delete(face::string_arg(),
                                       optional_arg(_bucket),
                                       _id),
                    failure_kind::query,
                    "gridfs_delete");

            return;
        });

    return;
}

std::optional<std::string> mongo_connection::gridfs_find_id(
    const std::string& _filename,
    const std::string& _bucket
)
{
    return this->run_guarded(
        [this, &_filename, &_bucket]() -> std::optional<std::string>
        {
            std::string id;

            internal::refuse_nul(_filename,
                                 "gridfs_find_id",
                                 "the file name");
            internal::refuse_nul(_bucket, "gridfs_find_id", "the bucket");

            const int result = m_db.gridfs_find_id(face::string_arg(),
                                                   optional_arg(_bucket),
                                                   _filename,
                                                   id);

            // no file of the name
            if (result == D_DB_NOT_FOUND)
            {
                return std::nullopt;
            }

            m_check(result, failure_kind::query, "gridfs_find_id");

            return id;
        });
}

std::unique_ptr<buffered_result_set> mongo_connection::gridfs_find(
    const std::string& _filter,
    const std::string& _options,
    const std::string& _bucket
)
{
    return this->run_guarded(
        [this, &_filter, &_options, &_bucket]()
            -> std::unique_ptr<buffered_result_set>
        {
            face::rows found;

            internal::refuse_nul(_filter, "gridfs_find", "the filter");
            internal::refuse_nul(_options, "gridfs_find", "the options");
            internal::refuse_nul(_bucket, "gridfs_find", "the bucket");
            m_check(m_db.gridfs_find(face::string_arg(),
                                     optional_arg(_bucket),
                                     optional_arg(_filter),
                                     optional_arg(_options),
                                     found),
                    failure_kind::query,
                    "gridfs_find");

            return m_rows(std::move(found));
        });
}

// 6.4.1.6  Upkeep
re_std::int64_t mongo_connection::get_collection_size(
    const std::string& _collection
) const
{
    return this->ask_guarded(
        [this, &_collection]() -> re_std::int64_t
        {
            re_std::int64_t bytes = 0;

            internal::refuse_nul(_collection,
                                 "get_collection_size",
                                 "the collection");
            m_check(m_db.collection_size(face::string_arg(),
                                         _collection,
                                         bytes),
                    failure_kind::query,
                    "get_collection_size");

            return bytes;
        });
}

re_std::int64_t mongo_connection::get_database_size() const
{
    return this->ask_guarded(
        [this]() -> re_std::int64_t
        {
            re_std::int64_t bytes = 0;

            m_check(m_db.database_size(face::string_arg(), bytes),
                    failure_kind::query,
                    "get_database_size");

            return bytes;
        });
}

void mongo_connection::compact(
    const std::string& _collection
)
{
    this->run_guarded(
        [this, &_collection]() -> void
        {
            internal::refuse_nul(_collection, "compact", "the collection");
            m_check(m_db.compact(face::string_arg(), _collection),
                    failure_kind::query,
                    "compact");

            return;
        });

    return;
}

mongo_validation mongo_connection::validate(
    const std::string& _collection,
    bool               _full
) const
{
    return this->ask_guarded(
        [this, &_collection, _full]() -> mongo_validation
        {
            mongo_validation found;

            internal::refuse_nul(_collection, "validate", "the collection");
            m_check(m_db.validate(
                        face::string_arg(),
                        _collection,
                        (_full)
                            ? static_cast<unsigned>(D_MONGODB_VALIDATE_FULL)
                            : 0u,
                        found.valid,
                        found.report),
                    failure_kind::query,
                    "validate");

            return found;
        });
}

// 6.4.1.7  Settings
/*
mongo_connection::set_read_concern
  "" is the layer's NULL: the deployment's own default.
*/
void mongo_connection::set_read_concern(
    const std::string& _concern
)
{
    this->run_guarded(
        [this, &_concern]() -> void
        {
            internal::refuse_nul(_concern, "set_read_concern", "the concern");
            m_check(m_db.set_read_concern(optional_arg(_concern)),
                    failure_kind::query,
                    "set_read_concern");

            return;
        });

    return;
}

void mongo_connection::set_read_concern(
    mongo_read_concern _level
)
{
    set_read_concern(std::string("{\"level\": \"") +
                     read_concern_name(_level) + "\"}");

    return;
}

std::string mongo_connection::get_read_concern() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string concern;

            m_check(m_db.get_read_concern(concern),
                    failure_kind::query,
                    "get_read_concern");

            return concern;
        });
}

void mongo_connection::set_write_concern(
    const std::string& _concern
)
{
    this->run_guarded(
        [this, &_concern]() -> void
        {
            internal::refuse_nul(_concern,
                                 "set_write_concern",
                                 "the concern");
            m_check(m_db.set_write_concern(optional_arg(_concern)),
                    failure_kind::query,
                    "set_write_concern");

            return;
        });

    return;
}

void mongo_connection::set_write_concern(
    mongo_write_concern _concern
)
{
    set_write_concern((_concern == mongo_write_concern::majority)
                          ? std::string("{\"w\": \"majority\"}")
                          : std::string("{\"w\": 1}"));

    return;
}

std::string mongo_connection::get_write_concern() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string concern;

            m_check(m_db.get_write_concern(concern),
                    failure_kind::query,
                    "get_write_concern");

            return concern;
        });
}

void mongo_connection::set_read_preference(
    const std::string& _preference
)
{
    this->run_guarded(
        [this, &_preference]() -> void
        {
            internal::refuse_nul(_preference,
                                 "set_read_preference",
                                 "the preference");
            m_check(m_db.set_read_preference(optional_arg(_preference)),
                    failure_kind::query,
                    "set_read_preference");

            return;
        });

    return;
}

void mongo_connection::set_read_preference(
    mongo_read_preference _mode
)
{
    set_read_preference(std::string("{\"mode\": \"") +
                        read_preference_name(_mode) + "\"}");

    return;
}

std::string mongo_connection::get_read_preference() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string preference;

            m_check(m_db.get_read_preference(preference),
                    failure_kind::query,
                    "get_read_preference");

            return preference;
        });
}

void mongo_connection::set_document_format(
    mongo_document_format _format
)
{
    this->run_guarded(
        [this, _format]() -> void
        {
            m_check(m_db.set_document_format(
                        static_cast<enum dj_mongodb_document_format>(_format)),
                    failure_kind::query,
                    "set_document_format");

            return;
        });

    return;
}

mongo_document_format mongo_connection::get_document_format() const noexcept
{
    return static_cast<mongo_document_format>(m_db.get_document_format());
}

void mongo_connection::set_max_time_ms(
    re_std::int64_t _milliseconds
)
{
    this->run_guarded(
        [this, _milliseconds]() -> void
        {
            m_check(m_db.set_max_time_ms(_milliseconds),
                    failure_kind::query,
                    "set_max_time_ms");

            return;
        });

    return;
}

re_std::int64_t mongo_connection::get_max_time_ms() const noexcept
{
    return m_db.get_max_time_ms();
}

// 6.4.1.8  The server, the driver and errors
unsigned long mongo_connection::server_version_number() const noexcept
{
    return m_db.server_version_number();
}

bool mongo_connection::server_at_least(
    unsigned _major,
    unsigned _minor,
    unsigned _patch
) const noexcept
{
    return m_db.server_at_least(_major, _minor, _patch);
}

int mongo_connection::server_wire_version() const noexcept
{
    return m_db.server_wire_version();
}

mongo_topology mongo_connection::get_server_topology() const noexcept
{
    return static_cast<mongo_topology>(m_db.server_topology());
}

std::string mongo_connection::get_replica_set_name() const
{
    const char* const name = m_db.server_replica_set();

    return (name) ? name
                  : "";
}

bool mongo_connection::supports_transactions() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_TRANSACTIONS);
}

bool mongo_connection::supports_change_streams() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_CHANGE_STREAMS);
}

bool mongo_connection::supports_time_series() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_TIME_SERIES);
}

bool mongo_connection::supports_clustered_collections() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_CLUSTERED);
}

bool mongo_connection::supports_stable_api() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_STABLE_API);
}

bool mongo_connection::supports_snapshot_reads() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_SNAPSHOT_READS);
}

bool mongo_connection::supports_pre_post_images() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_PRE_POST_IMAGES);
}

bool mongo_connection::supports_bulk_write() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_BULK_WRITE);
}

bool mongo_connection::supports_hidden_indexes() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_HIDDEN_INDEXES);
}

bool mongo_connection::supports_wildcard_indexes() const noexcept
{
    return m_supports(D_MONGODB_FEATURE_WILDCARD_INDEXES);
}

std::string mongo_connection::get_client_version()
{
    return face::mongodb::client_version();
}

unsigned long mongo_connection::get_client_version_number() noexcept
{
    return face::mongodb::client_version_number();
}

std::string mongo_connection::get_bson_version()
{
    return face::mongodb::bson_version();
}

void mongo_connection::set_log_level(
    int _level
)
{
    const int result = face::mongodb::set_log_level(_level);

    // no handle holds the failure's words
    if (result != 0)
    {
        mongodb_layer::raise(nullptr,
                             failure_kind::query,
                             "set_log_level",
                             result);
    }

    return;
}

int mongo_connection::get_log_level() noexcept
{
    return face::mongodb::get_log_level();
}

std::string mongo_connection::get_error_name() const
{
    const char* const name = m_db.error_name();

    return (name) ? name
                  : "";
}

bool mongo_connection::has_error_label(
    const std::string& _label
) const
{
    internal::refuse_nul(_label, "has_error_label", "the label");

    return m_db.error_has_label(_label);
}

/*
mongo_connection::get_error_reply
  No reply, or no handle, is nothing; only memory running out is a failure.
*/
std::optional<std::string> mongo_connection::get_error_reply() const
{
    std::string reply;
    const int   result = m_db.error_reply(reply);

    // no reply, or no handle
    if ( (result == D_DB_NOT_FOUND)        ||
         (result == D_DB_INVALID_ARGUMENT) ||
         (result == D_DB_UNSUPPORTED) )
    {
        return std::nullopt;
    }

    m_check(result, failure_kind::query, "get_error_reply");

    return reply;
}

std::optional<std::string> mongo_connection::get_last_inserted_id() const
{
    return this->ask_guarded(
        [this]() -> std::optional<std::string>
        {
            std::string id;
            const int   result = m_db.last_insert_id(id);

            // no new document yet
            if (result == D_DB_NOT_FOUND)
            {
                return std::nullopt;
            }

            m_check(result, failure_kind::query, "get_last_inserted_id");

            return id;
        });
}

void* mongo_connection::get_native_client() const noexcept
{
    return m_db.native();
}

/*
mongo_connection::get_mongo_config
  base is the connection's generic configuration, which connect(config) and
use_database change; the copy kept here is only what was last set or
connected with.
*/
mongo_connect_config mongo_connection::get_mongo_config() const
{
    mongo_connect_config config = m_mongo_config;

    config.base = this->m_config;

    return config;
}

void mongo_connection::set_mongo_config(
    const mongo_connect_config& _config
)
{
    m_mongo_config = _config;
    this->m_config = _config.base;

    return;
}

face::mongodb& mongo_connection::get_face() noexcept
{
    return m_db;
}

const face::mongodb& mongo_connection::get_face() const noexcept
{
    return m_db;
}

// hooks
/*
mongo_connection::connect_impl
  The handle is made in a face of its own -- from the configuration, or
over the client adopt() handed in -- and the object takes it only once it
is open, so that a failure leaves the old state and the new handle closed.
The current database, which a connection string may name, is the
configuration's from then on.
*/
void mongo_connection::connect_impl()
{
    face::mongodb opened;

    m_mongo_config.base = this->m_config;

    // a client of the program's, or the configuration's
    if (m_adopting.has_value())
    {
        const int result = opened.adopt(m_adopting->native,
                                        m_adopting->owned);

        if (result != 0)
        {
            mongodb_layer::raise(nullptr,
                                 failure_kind::connection,
                                 "adopt",
                                 result);
        }
    }
    else
    {
        m_connect(opened);
    }

    std::string current;

    // the database the handle is in, where it is in one
    if ( (opened.current_database(current) == 0) &&
         (!current.empty()) )
    {
        this->m_config.database = current;
    }

    m_db = std::move(opened);

    return;
}

void mongo_connection::disconnect_impl() noexcept
{
    (void)m_db.close();

    return;
}

bool mongo_connection::is_connected_impl() const noexcept
{
    return m_db.is_open();
}

/*
mongo_connection::ping_impl
  A ping is a round trip that changes nothing a caller can see, and the
face's is not const, so the face is reached through const_cast.
*/
bool mongo_connection::ping_impl() const noexcept
{
    return (const_cast<face::mongodb&>(m_db).ping() == 0);
}

face::rows mongo_connection::execute_query_impl(
    const std::string& _command
)
{
    face::rows found;

    m_check(m_db.query_rows(face::string_arg(), found, _command),
            failure_kind::query,
            "execute_query");

    return found;
}

re_std::int64_t mongo_connection::execute_update_impl(
    const std::string& _command
)
{
    re_std::int64_t changes = 0;

    m_check(m_db.exec_changes(face::string_arg(), changes, _command),
            failure_kind::query,
            "execute_update");

    return changes;
}

std::string mongo_connection::get_server_version_impl() const
{
    const char* const version = m_db.server_version();

    return (version) ? version
                     : "";
}

/*
mongo_connection::get_last_insert_id_impl
  An _id is any value, an ObjectId unless the document gave one; only an
integer is a number, and get_last_inserted_id gives any as JSON. None yet
answers 0, as the other vendors do.
*/
re_std::int64_t mongo_connection::get_last_insert_id_impl() const
{
    std::string     id;
    re_std::int64_t number = 0;
    const int       result = m_db.last_insert_id(id);

    // no new document yet
    if (result == D_DB_NOT_FOUND)
    {
        return 0;
    }

    m_check(result, failure_kind::query, "get_last_insert_id");

    // an integer, or an _id no number says
    if (!integer_id(id, number))
    {
        throw query_exception("MongoDB get_last_insert_id: the _id, " + id +
                                  ", is no integer; get_last_inserted_id "
                                  "gives it as JSON",
                              D_DB_MISMATCH);
    }

    return number;
}

void mongo_connection::begin_transaction_impl()
{
    m_check(m_db.begin_transaction(),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

void mongo_connection::commit_impl()
{
    m_check(m_db.commit(), failure_kind::transaction, "commit");

    return;
}

void mongo_connection::rollback_impl()
{
    m_check(m_db.rollback(), failure_kind::transaction, "rollback");

    return;
}

bool mongo_connection::in_transaction_impl() const noexcept
{
    return m_db.in_transaction();
}

struct ::dj_mongodb* mongo_connection::native_handle_impl() const noexcept
{
    return m_db.handle();
}

// private
void mongo_connection::m_check(
    int          _result,
    failure_kind _kind,
    const char*  _operation
) const
{
    // only a failure throws
    if (_result != 0)
    {
        mongodb_layer::raise(m_db.handle(), _kind, _operation, _result);
    }

    return;
}

void mongo_connection::m_wrote() noexcept
{
    this->record_changes(m_db.changes());

    return;
}

bool mongo_connection::m_supports(
    dj_mongodb_feature _feature
) const noexcept
{
    return m_db.supports(_feature);
}

/*
mongo_connection::m_document
  A document is text in JSON's formats and bytes in BSON's; either is the
std::string's bytes.
*/
std::optional<std::string> mongo_connection::m_document(
    const face::rows& _rows
)
{
    const struct dj_db_value* const cell =
        ( (_rows.row_count() > 0u) &&
          (_rows.column_count() > 0u) ) ? _rows.value(0u, 0u)
                                        : nullptr;

    // no row, or a value that is no document
    if ( (!cell) ||
         ( (cell->type != D_DB_VALUE_TEXT) &&
           (cell->type != D_DB_VALUE_BLOB) ) )
    {
        return std::nullopt;
    }

    return (cell->type == D_DB_VALUE_TEXT)
               ? std::string(cell->as.text, cell->size)
               : std::string(static_cast<const char*>(cell->as.blob),
                             cell->size);
}

/*
mongo_connection::m_modified
  A find-and-modify's no document is the layer's D_DB_NOT_FOUND, its write
made all the same.
*/
std::optional<std::string> mongo_connection::m_modified(
    int               _result,
    const face::rows& _found,
    const char*       _operation
)
{
    m_wrote();

    // none matched, or the upsert's document as it was, which is none
    if (_result == D_DB_NOT_FOUND)
    {
        return std::nullopt;
    }

    m_check(_result, failure_kind::query, _operation);

    return m_document(_found);
}

std::unique_ptr<buffered_result_set> mongo_connection::m_rows(
    face::rows&& _rows
)
{
    return std::make_unique<buffered_result_set>(std::move(_rows));
}

std::unique_ptr<mongo_cursor> mongo_connection::m_cursor(
    face::mongodb_cursor&& _opened
)
{
    return std::unique_ptr<mongo_cursor>(
        new mongo_cursor(this->share_session(),
                         m_db.handle(),
                         std::move(_opened)));
}

std::unique_ptr<mongo_change_stream> mongo_connection::m_stream(
    face::mongodb_change_stream&& _opened
)
{
    return std::unique_ptr<mongo_change_stream>(
        new mongo_change_stream(this->share_session(),
                                m_db.handle(),
                                std::move(_opened)));
}

/*
mongo_connection::m_connect
  The settings point into texts kept beside them, alive through the call.
*/
void mongo_connection::m_connect(
    face::mongodb& _opened
)
{
    struct dj_mongodb_options options = { nullptr, nullptr, nullptr, nullptr,
                                         0u,      nullptr, nullptr, nullptr };
    connect_settings         settings;

    fill_where(m_mongo_config, options);
    fill_base_settings(m_mongo_config.base, settings);
    fill_named_settings(m_mongo_config, settings);
    fill_option_settings(m_mongo_config, settings);

    face::text reason;
    const int  result = _opened.connect_with(&options,
                                             settings.data(),
                                             settings.size(),
                                             reason);

    // a failure, in the words of whoever refused it
    if (result != 0)
    {
        throw connection_exception(
            std::string("MongoDB connect: ") +
                ( (reason.is_null()) ? d_mongodb_result_message(result)
                                     : reason.c_str() ) +
                " (" + code_text(nullptr, result) + ")",
            result);
    }

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
