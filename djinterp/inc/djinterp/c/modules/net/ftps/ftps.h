/*******************************************************************************
* djinterp [c][net]                                                       ftps.h
*
* FTPS (RFC 4217): the FTP client, secured with TLS.
*   An FTPS client is an FTP client (net/ftp/ftp_client.h) and the security
* layer it runs its connections through: TLS sessions of the TLS kernel
* (net/tls/tls.h), one over the control connection and one over the data
* connection. The FTP client speaks RFC 2228's AUTH, PBSZ, PROT and CCC; this
* module gives it the mechanism they agree on, TLS: explicitly, after AUTH
* TLS, or implicitly, from the first byte on port 990; and, after PBSZ 0 and
* PROT P, on every data connection, where the client is always the TLS
* client (RFC 4217 section 7) in passive and active mode alike.
*   The TLS library is whatever engine the caller's dj_tls_context was made
* with. Control and data sessions come from that one context and name the
* same host, so an engine that caches client sessions by host resumes the
* control session on each data connection, as servers such as vsftpd require
* by default; each data session is offered the control session to resume as
* well. d_ftps_client_tls_config() fills a kernel configuration from the
* options.
*   A context whose plan checks less than the options ask is refused before
* connecting, and a call that reports D_FTP_ERROR_TLS leaves the kernel's
* status for d_ftps_client_tls_status. A client holds two TLS sessions and
* the FTP client's buffers, so it is large: give it static or heap storage,
* and do not copy it once initialized.
*
* HOW TO USE
* ----------
*     struct dj_ftp_options o;
*     struct dj_tls_config  c;
*     struct dj_tls_context ctx;
*     static struct dj_ftps_client client;
*     d_ftp_options_init(&o);
*     o.security = D_FTP_SECURITY_EXPLICIT;
*     d_ftps_client_tls_config(&o, &c);
*     d_tls_context_init(&ctx, engine, &c);
*     d_ftps_client_init(&client, &o, &ctx);
*     d_ftp_client_connect(&client.ftp, "ftp.example.com", 0);
*     d_ftp_client_login(&client.ftp);
*     ...
*     d_ftp_client_quit(&client.ftp);
*
* OPTIONS
* -------
*   The FTP client's (ftp_options.h): security, require_security,
* data_protection, verify_peer and verify_host are the ones TLS honours.
*
* CONFIGURATION
* -------------
*   None of its own: the TLS kernel's engines (cfg_tls.h) and the FTP
* client's buffers (cfg_ftp.h). Where D_INTERNAL_NET_HAS_INT64 is 0, with no
* 64-bit integer type, it compiles to nothing, unless
* D_CFG_NET_REQUIRE_BACKENDS asks for an #error (decision 101 of the
* register).
*
*
* path:      /inc/djinterp/c/modules/net/ftps/ftps.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Clients
         1.  dj_ftps_client
2.  CLIENT
    ------
    1.  Setup
    2.  Diagnostics
*/

#ifndef DJINTERP_C_NET_FTPS_H
#define DJINTERP_C_NET_FTPS_H 1

// djinterp
#include "../../../djinterp.h"  // framework root
#include "../../../../config/c/modules/net/cfg_net.h"  // D_INTERNAL_NET_*
#include "../ftp/ftp_client.h"   // dj_ftp_client
#include "../ftp/ftp_options.h"  // dj_ftp_options
#include "../net_common.h"       // D_INTERNAL_NET_HAS_INT64
#include "../tls/tls.h"          // dj_tls_context, dj_tls_session

// 64-bit floor: as ftp_client.h's, whose client this one holds.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftps.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Clients
//------------------------------------------------------------------------------
// 1.1.1
// dj_ftps_client
//   struct: an FTPS client: the FTP client every d_ftp_client_* call takes,
// the context its sessions come from, and the two sessions, each valid while
// its connection's `secured` is set. `ftp`'s public fields may be read as an
// FTP client's; the rest are the module's own.
struct dj_ftps_client
{
    struct dj_ftp_client         ftp;          // the FTP client
    const struct dj_tls_context* tls;          // borrowed; NULL: plain only
    struct dj_tls_session        control_tls;  // over ftp.control
    struct dj_tls_session        data_tls;     // over ftp.data
};


//==============================================================================
// 2.  CLIENT
//==============================================================================


// 2.1    Setup
//------------------------------------------------------------------------------
// d_ftps_client_tls_config() fills a kernel configuration for an FTPS client:
// a client's, verifying the server's chain and its name as the options'
// verify_peer and verify_host ask, against the system trust store. Set trust
// anchors or pins on it before d_tls_context_init(). NULL arguments are
// ignored.
void d_ftps_client_tls_config(
    const struct dj_ftp_options* _options,
    struct dj_tls_config*        _config
);

/**
 * @brief Prepares an FTPS client: closed, with its options copied, and its
 *        FTP client's security layer the TLS sessions of `_tls`.
 *
 * @param[out] _client  the client.
 * @param[in]  _options the options, copied; NULL for d_ftp_options_init()'s
 *                      defaults. The strings they point to must outlive the
 *                      client.
 * @param[in]  _tls     the TLS context, a client's, borrowed for the
 *                      client's life; NULL allows plain FTP only.
 */
void d_ftps_client_init(
    struct dj_ftps_client*       _client,
    const struct dj_ftp_options* _options,
    const struct dj_tls_context* _tls
);


// 2.2    Diagnostics
//------------------------------------------------------------------------------
// d_ftps_client_tls_status() gives the kernel's status for the last call on
// the client's `ftp` that reported D_FTP_ERROR_TLS, and D_TLS_OK before any;
// D_TLS_ERROR_INVALID_ARGUMENT for a NULL client.
enum dj_tls_error d_ftps_client_tls_status(
    const struct dj_ftps_client* _client
);


D_EXTERN_C_END


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "ftps.h: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_C_NET_FTPS_H
