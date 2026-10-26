/*******************************************************************************
* djinterp [core]                                                    mariadb.cpp
*
* Definitions for mariadb.hpp: mariadb_connection's constructors,
* configuration and hooks, and the MySQL family's members instantiated for
* it.
*   Everything the connection does is mysql_common_connection's, over the
* MariaDB layer's face; what MariaDB adds is its configuration, applied by
* the client within the connect, as init commands: wsrep_sync_wait for
* Galera's causal reads, and the session's default storage engine.
*
*
* path:      /src/djinterp/core/db/mariadb/mariadb.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb.h"  // d_mariadb_*
#include "../../../../../inc/djinterp/core/db/mysql/mysql_common.hpp"
                                       // mysql_common_connection's members


NS_DJINTERP

//==============================================================================
// 7.  MARIADB CONNECTION
//==============================================================================

template class mysql_common_connection<mariadb_connection,
                                       database_type::mariadb>;

mariadb_connection::mariadb_connection()
    : base_type(),
      m_mariadb_config()
{
    m_mariadb_config.mysql_config = this->m_mysql_config;
}

mariadb_connection::mariadb_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_mariadb_config(_config)
{
    m_mariadb_config.mysql_config = this->m_mysql_config;
}

mariadb_connection::mariadb_connection(
    const mysql_connect_config& _config
)
    : base_type(_config),
      m_mariadb_config()
{
    m_mariadb_config.mysql_config = this->m_mysql_config;
}

mariadb_connection::mariadb_connection(
    const mariadb_connect_config& _config
)
    : base_type(_config.mysql_config),
      m_mariadb_config(_config)
{
    m_mariadb_config.mysql_config = this->m_mysql_config;
}

/*
mariadb_connection::get_mariadb_config
  The family's settings, and the generic configuration in them, as they now
are (get_mysql_config); the copy kept here is only what was set.
*/
mariadb_connect_config
mariadb_connection::get_mariadb_config() const
{
    mariadb_connect_config config = m_mariadb_config;

    config.mysql_config = this->get_mysql_config();

    return config;
}

void
mariadb_connection::set_mariadb_config(
    const mariadb_connect_config& _config
)
{
    this->set_mysql_config(_config.mysql_config);

    m_mariadb_config              = _config;
    m_mariadb_config.mysql_config = this->m_mysql_config;

    return;
}

/*
mariadb_connection::add_settings_impl
  Galera's causal reads and the session's default storage engine, as init
commands: the client runs them within the connect -- so that connect_async
never waits for them -- after the configuration's own init command, a server
that refuses either failing the connect; a reset runs them again. An
engine's name is an identifier, refused otherwise, since no connection is
there yet to quote it.
*/
void
mariadb_connection::add_settings_impl(
    internal::mariadb_settings& _settings
) const
{
    const std::string& engine = m_mariadb_config.default_storage_engine;

    // Galera's causal reads, by either name
    if ( (m_mariadb_config.galera_wsrep_sync_wait) ||
         (m_mariadb_config.galera_wsrep_causal_reads) )
    {
        _settings.add_text(D_MARIADB_SETTING_INIT_COMMAND,
                           "SET SESSION wsrep_sync_wait = 1");
    }

    // the session's default storage engine, where one is named
    if (!engine.empty())
    {
        for (const char letter : engine)
        {
            const bool word = ( ( (letter >= 'a') && (letter <= 'z') ) ||
                                ( (letter >= 'A') && (letter <= 'Z') ) ||
                                ( (letter >= '0') && (letter <= '9') ) ||
                                (letter == '_') );

            if (!word)
            {
                throw connection_exception(
                    "MariaDB connect: default_storage_engine '" + engine +
                        "' is not an engine's name",
                    D_DB_INVALID_ARGUMENT);
            }
        }

        _settings.add_text(D_MARIADB_SETTING_INIT_COMMAND,
                           "SET SESSION default_storage_engine = '" + engine +
                               "'");
    }

    return;
}

/*
mariadb_connection::on_connected_impl
  Nothing: the leaf's session set-up runs within the connect, as init
commands (add_settings_impl).
*/
void
mariadb_connection::on_connected_impl(
    face::mariadb& _db
)
{
    static_cast<void>(_db);

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
