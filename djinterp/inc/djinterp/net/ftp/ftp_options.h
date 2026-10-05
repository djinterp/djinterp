/*******************************************************************************
* djinterp [net]                                                   ftp_options.h
*
* FTP client options.
*   Backend-neutral settings for a client session -- credentials, security,
* data-connection mode, representation type, verification, and timeouts --
* with defaults that favor safety.
*
*
* path:      /inc/djinterp/net/ftp/ftp_options.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Options
         1.  d_ftp_options
2.  CLIENT OPTIONS
    --------------
    1.  Defaults
*/

#ifndef DJINTERP_NET_FTP_FTP_OPTIONS_H
#define DJINTERP_NET_FTP_FTP_OPTIONS_H 1

// std
#include <stdint.h>  // uint32_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_transfer.h"    // d_ftp_type
#include "./ftp_endpoint.h"    // d_ftp_data_mode
#include "./ftp_security.h"    // d_ftp_security, d_ftp_protection


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_options.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Options
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_options
//   struct: backend-neutral client settings. Each backend honors what its
// library can express; one that cannot honor a security-relevant setting must
// refuse to connect rather than silently weaken it.
struct d_ftp_options
{
    const char*           user;                 // NULL: "anonymous"
    const char*           password;             // NULL: backend default
    const char*           account;              // ACCT, if 332 asks for it
    enum d_ftp_security   security;             // TLS on the control link
    bool                  require_security;     // no fallback to plain FTP
    enum d_ftp_protection data_protection;      // PROT level under TLS
    bool                  clear_control;        // CCC after logging in
    enum d_ftp_data_mode  data_mode;            // passive or active
    struct d_ftp_type     type;                 // representation type
    bool                  ignore_pasv_address;  // use the control peer
    bool                  verify_peer;          // check the certificate
    bool                  verify_host;          // check the host name
    bool                  use_utf8;             // OPTS UTF8 ON if offered
    bool                  create_directories;   // MKD missing parents
    uint32_t              connect_timeout_ms;   // 0: no limit
    uint32_t              response_timeout_ms;  // 0: no limit
    uint32_t              idle_timeout_ms;      // 0: no limit
};


//==============================================================================
// 2.  CLIENT OPTIONS
//==============================================================================


// 2.1    Defaults
//------------------------------------------------------------------------------
/**
 * @brief Fills options with safe defaults: anonymous login, no TLS but no
 *        silent fallback when TLS is requested, PROT P, extended passive mode
 *        with PASV fallback, binary (TYPE I) transfers, the PASV address
 *        ignored, certificates and host names verified, UTF-8 when offered,
 *        a 30-second connect timeout, and a 60-second response timeout.
 *
 * @param[out] _options the options to fill.
 */
void d_ftp_options_init(struct d_ftp_options* _options);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_OPTIONS_H
