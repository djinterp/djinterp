/*******************************************************************************
* djinterp [core]                                                    mariadb.cpp
*
* Definitions for mariadb.hpp: mariadb_connection's constructors,
* configuration and hooks, and the MySQL family's members instantiated for
* it.
*   Everything the connection does is mysql_common_connection's, over the
* MariaDB layer's face; what MariaDB adds is its configuration, applied once
* the connection opens and before the object takes it: wsrep_sync_wait for
* Galera's causal reads, and the session's default storage engine.
*
*
* path:      /src/djinterp/core/db/mariadb/mariadb.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.16
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
// VIII. MARIADB CONNECTION
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

const mariadb_connect_config&
mariadb_connection::get_mariadb_config() const noexcept
{
    return m_mariadb_config;
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

void
mariadb_connection::add_settings_helper(
    internal::mariadb_settings& _settings
) const
{
    static_cast<void>(_settings);

    return;
}

/*
mariadb_connection::on_connected_helper
  Both run on the new connection, before the object takes it: a server that
refuses either leaves the object as it was, and the new connection closes as
the face holding it goes. An engine's name is quoted as a literal, since the
layer takes a variable's value as SQL.
*/
void
mariadb_connection::on_connected_helper(
    face::mariadb& _db
)
{
    // Galera's causal reads, by either name
    if ( (m_mariadb_config.galera_wsrep_sync_wait) ||
         (m_mariadb_config.galera_wsrep_causal_reads) )
    {
        const int result = _db.set_variable("wsrep_sync_wait", "1");

        if (result != 0)
        {
            mariadb_layer::raise(_db.handle(),
                                 failure_kind::connection,
                                 "wsrep_sync_wait",
                                 result);
        }
    }

    // the session's default storage engine, where one is named
    if (!m_mariadb_config.default_storage_engine.empty())
    {
        face::text quoted;

        _db.quote_literal(m_mariadb_config.default_storage_engine, quoted);

        const int result =
            (quoted.is_null())
                ? D_DB_INVALID_ARGUMENT
                : _db.set_variable("default_storage_engine", quoted.c_str());

        if (result != 0)
        {
            mariadb_layer::raise(_db.handle(),
                                 failure_kind::connection,
                                 "default_storage_engine",
                                 result);
        }
    }

    return;
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
