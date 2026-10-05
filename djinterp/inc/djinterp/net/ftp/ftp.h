/*******************************************************************************
* djinterp [net]                                                           ftp.h
*
* Umbrella header for the FTP protocol foundation.
*   Includes every net/ftp/ module; include one module instead to take only
* what a translation unit uses. The protocol modules are library-agnostic
* C99; ftp_client adds the wire: the TCP transport (net/tcp/tcp.h) and, for
* FTPS, TLS through the SSL kernel (net/ssl/ssl.h) with whatever engine a
* caller plugs in. Other backends can share the protocol modules alone.
*   Environment detection -- which transports and libraries exist -- lives in
* env/net/ftp/env_ftp.h and, for FTPS, env/net/ftps/env_ftps.h; nothing under
* net/ftp/ needs it.
*
*
* path:      /inc/djinterp/net/ftp/ftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_NET_FTP_FTP_H
#define DJINTERP_NET_FTP_FTP_H 1

// djinterp
#include "./ftp_common.h"    // ports, spans, buffers, errors
#include "./ftp_reply.h"     // reply codes, parsing, formatting
#include "./ftp_command.h"   // command vocabulary and lines
#include "./ftp_transfer.h"  // TYPE, STRU, MODE; ASCII conversion
#include "./ftp_endpoint.h"  // PORT, PASV, EPRT, EPSV
#include "./ftp_security.h"  // TLS modes and PROT levels
#include "./ftp_path.h"      // pathname quoting and resolution
#include "./ftp_fact.h"      // timestamps and sizes
#include "./ftp_feature.h"   // FEAT negotiation
#include "./ftp_listing.h"   // directory listings
#include "./ftp_url.h"       // URLs and percent-encoding
#include "./ftp_options.h"   // client options
#include "./ftp_client.h"    // FTP and FTPS client


#endif  // DJINTERP_NET_FTP_FTP_H
