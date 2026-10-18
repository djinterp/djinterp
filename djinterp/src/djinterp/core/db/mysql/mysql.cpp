/*******************************************************************************
* djinterp [core]                                                      mysql.cpp
*
* Definitions for mysql.hpp: mysql_connection's constructors, configuration
* and hooks, and the MySQL family's members instantiated for it.
*   Everything the connection does is mysql_common_connection's, over the
* MariaDB layer's face; what MySQL adds is its configuration: a TLS mode of
* its own, connect attributes, and the refusal of the settings the layer has
* nothing to apply them with, which a program sets with set_option instead.
*
*
* path:      /src/djinterp/core/db/mysql/mysql.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
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
 * @brief Refuses a setting the layer cannot apply, when it is set.
 *
 * @param[in] _set    whether the setting differs from its default.
 * @param[in] _field  the configuration's field.
 * @throws connection_exception with D_DB_UNSUPPORTED when `_set`.
 */
void
refuse_if_set(
    bool        _set,
    const char* _field
)
{
    // a setting left as it was made needs nothing
    if (_set)
    {
        throw connection_exception(std::string("MySQL connect: ") + _field +
                                   " is not supported; set its "
                                   "mysql_options option with set_option",
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
// V.   MYSQL CONNECTION
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

const oracle_mysql_connect_config&
mysql_connection::get_oracle_config() const noexcept
{
    return m_oracle_config;
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
mysql_connection::add_settings_helper
  The refusals come first, before anything is asked of the server: a setting
the program made and the layer cannot apply must not be dropped silently --
a TLS version or an authentication plugin that is not used is not a
preference but a requirement unmet.
*/
void
mysql_connection::add_settings_helper(
    internal::mariadb_settings& _settings
) const
{
    refuse_if_set(!m_oracle_config.auth_plugin.empty(), "auth_plugin");
    refuse_if_set(!m_oracle_config.tls_version.empty(), "tls_version");
    refuse_if_set(!m_oracle_config.tls_ciphersuites.empty(),
                  "tls_ciphersuites");
    refuse_if_set(m_oracle_config.use_x_protocol, "use_x_protocol");
    refuse_if_set(m_oracle_config.enable_session_tracking,
                  "enable_session_tracking");
    refuse_if_set(m_oracle_config.enable_cleartext_plugin,
                  "enable_cleartext_plugin");
    refuse_if_set(m_oracle_config.statement_cache_size != 0u,
                  "statement_cache_size");

    // a TLS mode of MySQL's own, which replaces enable_ssl's
    if (m_oracle_config.ssl_mode != mysql_ssl_mode::preferred)
    {
        _settings.add_number(
            D_MARIADB_SETTING_SSL_MODE,
            static_cast<unsigned long>(
                layer_ssl_mode(m_oracle_config.ssl_mode)));
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
mysql_connection::on_connected_helper(
    face::mariadb& _db
)
{
    static_cast<void>(_db);

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
