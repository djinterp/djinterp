/*******************************************************************************
* djinterp [core]                                                      mysql.cpp
*
* Definitions for mysql.hpp: mysql_connection's constructors, configuration,
* query attributes and hooks, and the MySQL family's members instantiated
* for it.
*   Everything the connection does is mysql_common_connection's, over the
* MariaDB layer's face; what MySQL adds is its configuration -- a TLS mode of
* its own, connect attributes, the TLS versions and cipher suites, the
* authentication plugin and the clear-text switch by the client's options
* where it has them, session tracking by an init command, and the refusal of
* the X protocol and a statement cache, which neither client has -- and the
* query attributes only its server reads.
*
*
* path:      /src/djinterp/core/db/mysql/mysql.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../../inc/djinterp/core/db/mysql/mysql.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>   // std::string
#include <utility>  // std::pair
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb.h"  // d_mariadb_*
#include "../../../../../inc/djinterp/core/db/mysql/mysql_common.hpp"
                                       // mysql_common_connection's members


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

/**
 * @brief Refuses a setting no client can apply, when it is set.
 *
 * @param[in] _set    whether the setting differs from its default.
 * @param[in] _field  the configuration's field.
 * @param[in] _why    why no client can.
 * @throws connection_exception with D_DB_UNSUPPORTED when `_set`.
 */
void
refuse_if_set(
    bool        _set,
    const char* _field,
    const char* _why
)
{
    // a setting left as it was made needs nothing
    if (_set)
    {
        throw connection_exception(std::string("MySQL connect: ") + _field +
                                   " is not supported: " + _why,
                                   D_DB_UNSUPPORTED);
    }

    return;
}

/**
 * @brief The layer's TLS mode for one of MySQL's.
 *
 * @param[in] _mode  MySQL's mode.
 * @return the layer's.
 */
enum d_mariadb_ssl_mode
layer_ssl_mode(
    mysql_ssl_mode _mode
) noexcept
{
    // the five both name
    switch (_mode)
    {
        case mysql_ssl_mode::disabled:
            return D_MARIADB_SSL_DISABLED;

        case mysql_ssl_mode::required:
            return D_MARIADB_SSL_REQUIRED;

        case mysql_ssl_mode::verify_ca:
            return D_MARIADB_SSL_VERIFY_CA;

        case mysql_ssl_mode::verify_identity:
            return D_MARIADB_SSL_VERIFY_IDENTITY;

        case mysql_ssl_mode::preferred:
        default:
            return D_MARIADB_SSL_PREFERRED;
    }
}

}  // namespace

//==============================================================================
// 4.  MYSQL CONNECTION
//==============================================================================

template class mysql_common_connection<mysql_connection,
                                       database_type::mysql>;

mysql_connection::mysql_connection()
    : base_type(),
      m_oracle_config()
{
    m_oracle_config.common = this->m_mysql_config;
}

mysql_connection::mysql_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_oracle_config(_config)
{
    m_oracle_config.common = this->m_mysql_config;
}

mysql_connection::mysql_connection(
    const mysql_connect_config& _config
)
    : base_type(_config),
      m_oracle_config()
{
    m_oracle_config.common = this->m_mysql_config;
}

mysql_connection::mysql_connection(
    const oracle_mysql_connect_config& _config
)
    : base_type(_config.common),
      m_oracle_config(_config)
{
    m_oracle_config.common = this->m_mysql_config;
}

/*
mysql_connection::get_oracle_config
  The family's settings, and the generic configuration in them, as they now
are (get_mysql_config); the copy kept here is only what was set.
*/
oracle_mysql_connect_config
mysql_connection::get_oracle_config() const
{
    oracle_mysql_connect_config config = m_oracle_config;

    config.common = this->get_mysql_config();

    return config;
}

void
mysql_connection::set_oracle_config(
    const oracle_mysql_connect_config& _config
)
{
    this->set_mysql_config(_config.common);

    m_oracle_config        = _config;
    m_oracle_config.common = this->m_mysql_config;

    return;
}

/*
mysql_connection::is_lts_release
  A series is MAJOR*100 + MINOR, from the server's version number; each
flavor's long-term-support series are env's (D_ENV_MYSQL_IS_LTS,
D_ENV_MARIADB_IS_LTS).
*/
bool
mysql_connection::is_lts_release() const noexcept
{
    // a server, which says which it is
    if (!this->is_connected())
    {
        return false;
    }

    const unsigned long series =
        this->get_face().server_version_number() / 100ul;

    switch (this->get_face().server_flavor())
    {
        case D_MARIADB_FLAVOR_MYSQL:
            return ( (series == 804ul) ||
                     (series == 907ul) );

        case D_MARIADB_FLAVOR_MARIADB:
            return ( (series == 1006ul) ||
                     (series == 1011ul) ||
                     (series == 1104ul) ||
                     (series == 1108ul) ||
                     (series == 1203ul) );

        case D_MARIADB_FLAVOR_UNKNOWN:
        default:
            return false;
    }
}

/*
mysql_connection::set_query_attribute
  The layer keeps its own copies until the next query is sent, so the
strings need not outlive the call; a NUL would end them early.
*/
void
mysql_connection::set_query_attribute(
    const std::string& _name,
    const std::string& _value
)
{
    this->run_guarded(
        [this, &_name, &_value]() -> void
        {
            internal::refuse_nul(_name, "set_query_attribute", "the name");
            internal::refuse_nul(_value, "set_query_attribute", "the value");
            this->m_check(this->get_face().set_query_attribute(_name, _value),
                          failure_kind::query,
                          "set_query_attribute");

            return;
        });

    return;
}

void
mysql_connection::clear_query_attributes()
{
    this->run_guarded(
        [this]() -> void
        {
            this->m_check(this->get_face().clear_query_attributes(),
                          failure_kind::query,
                          "clear_query_attributes");

            return;
        });

    return;
}

/*
mysql_connection::add_settings_impl
  The refusals come first, before anything is asked of the server: a setting
the program made and the layer cannot apply must not be dropped silently --
a TLS version or an authentication plugin that is not used is not a
preference but a requirement unmet. Neither client speaks the X protocol or
caches statements, so those two are refused here; each of the rest goes to
the client's own option, which the layer refuses, D_DB_UNSUPPORTED, where the
client has none. Session tracking is the server's, asked of it by an init
command, which a reset runs again.
*/
void
mysql_connection::add_settings_impl(
    internal::mariadb_settings& _settings
) const
{
    refuse_if_set(m_oracle_config.use_x_protocol,
                  "use_x_protocol",
                  "neither client speaks the X protocol");
    refuse_if_set(m_oracle_config.statement_cache_size != 0u,
                  "statement_cache_size",
                  "neither client keeps a cache of statements");

    // a TLS mode of MySQL's own, which replaces enable_ssl's
    if (m_oracle_config.ssl_mode != mysql_ssl_mode::preferred)
    {
        _settings.add_number(
            D_MARIADB_SETTING_SSL_MODE,
            static_cast<unsigned long>(
                layer_ssl_mode(m_oracle_config.ssl_mode)));
    }

    // the TLS versions and TLS 1.3's cipher suites allowed, the plugin to
    // answer with first, and the clear-text switch, by the client's options
    if (!m_oracle_config.tls_version.empty())
    {
        _settings.add_text(D_MARIADB_SETTING_TLS_VERSION,
                           m_oracle_config.tls_version);
    }

    if (!m_oracle_config.tls_ciphersuites.empty())
    {
        _settings.add_text(D_MARIADB_SETTING_TLS_CIPHERSUITES,
                           m_oracle_config.tls_ciphersuites);
    }

    if (!m_oracle_config.auth_plugin.empty())
    {
        _settings.add_text(D_MARIADB_SETTING_DEFAULT_AUTH,
                           m_oracle_config.auth_plugin);
    }

    if (m_oracle_config.enable_cleartext_plugin)
    {
        _settings.add_number(D_MARIADB_SETTING_CLEARTEXT_PLUGIN, 1ul);
    }

    // the server's RSA public key, from a file or asked of the server
    if (!m_oracle_config.server_public_key.empty())
    {
        _settings.add_text(D_MARIADB_SETTING_SERVER_PUBLIC_KEY,
                           m_oracle_config.server_public_key);
    }

    if (m_oracle_config.get_server_public_key)
    {
        _settings.add_number(D_MARIADB_SETTING_GET_SERVER_PUBLIC_KEY, 1ul);
    }

    // every kind of session state the server can report changes to
    if (m_oracle_config.enable_session_tracking)
    {
        _settings.add_text(D_MARIADB_SETTING_INIT_COMMAND,
                           "SET SESSION session_track_schema = ON, "
                           "session_track_state_change = ON, "
                           "session_track_system_variables = '*', "
                           "session_track_gtids = OWN_GTID, "
                           "session_track_transaction_info = "
                           "CHARACTERISTICS");
    }

    // MySQL's connect attributes, beside the family's
    for (const std::pair<const std::string, std::string>& attribute :
         m_oracle_config.connect_attributes)
    {
        _settings.add_attribute(attribute.first, attribute.second);
    }

    return;
}

void
mysql_connection::on_connected_impl(
    face::mariadb& _db
)
{
    static_cast<void>(_db);

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
