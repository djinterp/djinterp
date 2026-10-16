/*******************************************************************************
* djinterp [core]                                            database_common.cpp
*
*
* path:      /src/djinterp/core/db/database_common.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/core/db/database_common.hpp"


NS_DJINTERP
NS_DATABASE


connection_config::connection_config()
    : host("localhost"),
      port(0),
      connect_timeout(std::chrono::seconds(30)),
      read_timeout(std::chrono::seconds(30)),
      write_timeout(std::chrono::seconds(30)),
      auto_reconnect(false),
      enable_ssl(false),
      verify_ssl(true),
      max_packet_size(16 * 1024 * 1024)
{}

pool_config::pool_config()
    : min_connections(1),
      max_connections(10),
      connection_queue_size(100),
      connection_timeout(std::chrono::seconds(30)),
      idle_timeout(std::chrono::minutes(10)),
      max_lifetime(std::chrono::hours(1)),
      validate_on_acquire(false),
      validate_on_return(false)
{}

exception::exception(
    const std::string& _message
)
    : std::runtime_error(_message),
      m_message(_message),
      m_error_code(0)
{}

exception::exception(
    const char* _message
)
    : std::runtime_error(_message),
      m_message(_message),
      m_error_code(0)
{}

const char*
exception::what() const noexcept
{
    return m_message.c_str();
}

int
exception::error_code() const noexcept
{
    return m_error_code;
}


NS_END  // database
NS_END  // djinterp
