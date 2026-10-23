/*******************************************************************************
* djinterp [net]                                                  net_common.cpp
*
* The out-of-line members of net_common.hpp: the C++ connection interface's
* destructor, so its vtable has one home, and the two bridges to C --
* c_connection's operations and c_connection_adapter's thunks and table.
*   Everything else in net_common.hpp is inline or a template, and stays
* there.
*
*
* path:      /src/djinterp/net/net_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../inc/djinterp/net/net_common.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t


NS_DJINTERP
NS_NET

/*
connection::~connection
  The interface's one out-of-line member, so its vtable has one home, here.
*/
connection::~connection()
{}

/*
c_connection::~c_connection
  Destroys the owned C connection; a moved-from one holds none, which
d_net_connection_destroy ignores.
*/
c_connection::~c_connection()
{
    ::d_net_connection_destroy(m_connection);
}

/*
c_connection::read
  Forwards to net_common.h's dispatch and converts the result.
*/
io_result
c_connection::read(
    void*       _buffer,
    std::size_t _size
)
{
    return from_c(::d_net_connection_read(m_connection,
                                          _buffer,
                                          _size));
}

/*
c_connection::write
  Forwards to net_common.h's dispatch and converts the result.
*/
io_result
c_connection::write(
    const void* _data,
    std::size_t _size
)
{
    return from_c(::d_net_connection_write(m_connection,
                                           _data,
                                           _size));
}

/*
c_connection::is_open
  Forwards to net_common.h's dispatch.
*/
bool
c_connection::is_open() const
{
    return ::d_net_connection_is_open(m_connection);
}

/*
c_connection::close
  Forwards to net_common.h's dispatch.
*/
void
c_connection::close()
{
    ::d_net_connection_close(m_connection);

    return;
}

/*
c_connection::shutdown
  Converts the mode, forwards, and converts the error back.
*/
io_error::value
c_connection::shutdown(
    shutdown_mode::value _mode
)
{
    return from_c(::d_net_connection_shutdown(m_connection,
                                              to_c(_mode)));
}

/*
c_connection::remote_endpoint
  An address the C connection cannot give is the empty endpoint.
*/
endpoint
c_connection::remote_endpoint() const
{
    d_net_endpoint address = {};

    // an unknown address is the empty endpoint
    if (!::d_net_connection_remote_endpoint(m_connection,
                                            &address))
    {
        return endpoint();
    }

    return from_c(address);
}

/*
c_connection::local_endpoint
  An address the C connection cannot give is the empty endpoint.
*/
endpoint
c_connection::local_endpoint() const
{
    d_net_endpoint address = {};

    // an unknown address is the empty endpoint
    if (!::d_net_connection_local_endpoint(m_connection,
                                           &address))
    {
        return endpoint();
    }

    return from_c(address);
}

/*
c_connection_adapter::m_export
  Stores a known address; an empty host means unknown.
*/
bool
c_connection_adapter::m_export(
    const endpoint& _address,
    d_net_endpoint* _out
) D_NOEXCEPT
{
    return ( (!_address.host.empty()) &&
             (to_c(_address,
                   *_out) == io_error::none) );
}

/*
c_connection_adapter::m_read
  Each thunk finds its adapter from the C connection, the adapter's first
member, and forwards to the borrowed C++ connection.
*/
d_net_io_result
c_connection_adapter::m_read(
    d_net_connection* _connection,
    void*             _buffer,
    std::size_t       _size
) D_NOEXCEPT
{
    return to_c(m_of(_connection).m_target->read(_buffer,
                                                 _size));
}

/*
c_connection_adapter::m_write
  As m_read.
*/
d_net_io_result
c_connection_adapter::m_write(
    d_net_connection* _connection,
    const void*       _data,
    std::size_t       _size
) D_NOEXCEPT
{
    return to_c(m_of(_connection).m_target->write(_data,
                                                  _size));
}

/*
c_connection_adapter::m_is_open
  As m_read.
*/
bool
c_connection_adapter::m_is_open(
    const d_net_connection* _connection
) D_NOEXCEPT
{
    return m_of(_connection).m_target->is_open();
}

/*
c_connection_adapter::m_close
  As m_read.
*/
void
c_connection_adapter::m_close(
    d_net_connection* _connection
) D_NOEXCEPT
{
    m_of(_connection).m_target->close();

    return;
}

/*
c_connection_adapter::m_shutdown
  As m_read, converting the mode one way and the error the other.
*/
d_net_error
c_connection_adapter::m_shutdown(
    d_net_connection* _connection,
    d_net_shutdown    _mode
) D_NOEXCEPT
{
    return to_c(m_of(_connection).m_target->shutdown(from_c(_mode)));
}

/*
c_connection_adapter::m_remote_endpoint
  As m_read, exporting the address through m_export.
*/
bool
c_connection_adapter::m_remote_endpoint(
    const d_net_connection* _connection,
    d_net_endpoint*         _out
) D_NOEXCEPT
{
    return m_export(m_of(_connection).m_target->remote_endpoint(),
                    _out);
}

/*
c_connection_adapter::m_local_endpoint
  As m_read, exporting the address through m_export.
*/
bool
c_connection_adapter::m_local_endpoint(
    const d_net_connection* _connection,
    d_net_endpoint*         _out
) D_NOEXCEPT
{
    return m_export(m_of(_connection).m_target->local_endpoint(),
                    _out);
}

/*
c_connection_adapter::m_vtable
  The one table every adapter points at: no destroy, since the adapter's
storage belongs to its owner, and no available, since a C++ connection
cannot say how much is waiting.
*/
const d_net_connection_vtable&
c_connection_adapter::m_vtable() D_NOEXCEPT
{
    static const d_net_connection_vtable table =
    {
        &c_connection_adapter::m_read,
        &c_connection_adapter::m_write,
        &c_connection_adapter::m_is_open,
        &c_connection_adapter::m_close,
        &c_connection_adapter::m_shutdown,
        &c_connection_adapter::m_remote_endpoint,
        &c_connection_adapter::m_local_endpoint,
        D_NULLPTR,
        D_NULLPTR
    };

    return table;
}

NS_END  // net
NS_END  // djinterp
