/*******************************************************************************
* djinterp [cpp][net]                                                   ftps.hpp
*
* The C++ interface to the FTPS client.
*   A zero-cost view of ftps.h: `client` holds a dj_ftps_client, and its
* session's members are ftp::internal::client_base's, the plain client's,
* over the FTP client inside it; it adds what TLS has: the context it is
* made with, a kernel configuration from the options, and the kernel's
* status for a failure. The options, errors and callbacks are ftp's
* (ftp_client.hpp).
*   C++98 is the floor; C++11 only adds: copying is deleted rather than
* hidden, and the members are noexcept. Errors are values, never
* exceptions.
*
* HOW TO USE
* ----------
*     ftp::options o;
*     o.security(ftp::security::explicit_tls);
*     dj_tls_config c;
*     ftps::client::tls_config(o, c);
*     dj_tls_context ctx;
*     d_tls_context_init(&ctx, engine, &c);
*     ftps::client client(o, &ctx);
*     client.connect("ftp.example.com", 0);
*     client.login();
*     ...
*     client.quit();
*
* OPTIONS
* -------
*   ftp::options's: security, require_security, protection, verify_peer and
* verify_host are the ones TLS honours.
*
* CONFIGURATION
* -------------
*   None of its own: ftps.h's. Where D_INTERNAL_NET_HAS_INT64 is 0, with no
* 64-bit integer type, it compiles to nothing, unless
* D_CFG_NET_REQUIRE_BACKENDS asks for an #error (decision 101 of the
* register).
*
*
* path:      /inc/djinterp/cpp/modules/net/ftps/ftps.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLIENT
    ------
    1.  Sessions
         1.  client
*/

#ifndef DJINTERP_NET_FTPS_HPP
#define DJINTERP_NET_FTPS_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "../ftp/ftp_client.hpp"  // ftp::options, ftp::internal::client_base
#include "../../../../c/modules/net/ftp/ftp_client.h"  // dj_ftp_client
#include "../../../../c/modules/net/ftps/ftps.h"       // the C client
#include "../../../../c/modules/net/net_common.h"      // D_INTERNAL_NET_HAS_INT64
#include "../../../../c/modules/net/tls/tls.h"         // dj_tls_context, dj_tls_error
#include "../../../../config/c/modules/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS

// 64-bit floor: as ftp_client.hpp's.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


namespace djinterp {
namespace net {
namespace ftps {


//==============================================================================
// 1.  CLIENT
//==============================================================================


// 1.1    Sessions
//------------------------------------------------------------------------------
// 1.1.1
// client
//   class: an FTPS session, its members ftp::internal::client_base's; the
// destructor closes whatever is open, sending nothing. The C client holds
// pointers into itself, so a client can be neither copied nor moved: hold
// it where it is made, or on the heap.
class client : public ftp::internal::client_base<client>
{
public:
    client() D_NOEXCEPT
    {
        ::d_ftps_client_init(&m_client,
                             NULL,
                             NULL);
    }

    // _tls: the TLS context, a client's, borrowed for the client's
    // lifetime; NULL for plain FTP
    explicit
    client(
        const ftp::options&           _options,
        const ::dj_tls_context* const _tls = NULL
    ) D_NOEXCEPT
    {
        ::d_ftps_client_init(&m_client,
                             &_options.c_options(),
                             _tls);
    }

    ~client()
    {
        ::d_ftp_client_close(&m_client.ftp);
    }

    // tls_config - fills a TLS configuration from the options
    static
    void tls_config(
        const ftp::options& _options,
        ::dj_tls_config&    _config
    ) D_NOEXCEPT
    {
        ::d_ftps_client_tls_config(&_options.c_options(),
                                   &_config);
    }

    // tls_status - the kernel's status for the last ERROR_TLS
    ::dj_tls_error tls_status() const D_NOEXCEPT
    {
        return ::d_ftps_client_tls_status(&m_client);
    }

    // c_client - the dj_ftp_client beneath, for what has no member here
    ::dj_ftp_client& c_client() D_NOEXCEPT
    {
        return m_client.ftp;
    }

    const ::dj_ftp_client& c_client() const D_NOEXCEPT
    {
        return m_client.ftp;
    }

    // c_ftps_client - the dj_ftps_client beneath
    ::dj_ftps_client& c_ftps_client() D_NOEXCEPT
    {
        return m_client;
    }

    const ::dj_ftps_client& c_ftps_client() const D_NOEXCEPT
    {
        return m_client;
    }

private:
    D_DELETED_FN(client(const client&))
    D_DELETED_FN(client& operator=(const client&))

    ::dj_ftps_client m_client;
};


}  // namespace ftps
}  // namespace net
}  // namespace djinterp


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "ftps.hpp: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_NET_FTPS_HPP
