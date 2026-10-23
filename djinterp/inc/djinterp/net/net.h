/*******************************************************************************
* djinterp [net]                                                           net.h
*
* The net subframework's umbrella: including it includes every C module of
* net, with the foundation they share, net_common.h.
*   Each module's own header can be included alone instead; this one is for a
* program that wants them all. A header whose backend or floor is absent
* compiles to nothing, unless cfg_net.h's D_CFG_NET_REQUIRE_BACKENDS asks for
* an #error.
*
*
* path:      /inc/djinterp/net/net.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_NET_NET_H
#define DJINTERP_NET_NET_H 1

// djinterp
#include "./net_common.h"         // errors, endpoints, connections, streams
#include "./net_memory.h"         // rings, pipes, memory streams
#include "./net_url.h"            // URLs, RFC 3986
#include "./codec/codec.h"        // ASCII, hex, base64, lines, digests
#include "./tcp/tcp.h"            // TCP connections and listeners
#include "./ssl/ssl.h"            // the SSL kernel
#include "./ssl/ssl_openssl.h"    // its OpenSSL-family engine
#include "./starttls/starttls.h"  // STARTTLS
#include "./http/http.h"          // HTTP's vocabulary
#include "./http/http1.h"         // HTTP/1.1's message syntax
#include "./ftp/ftp.h"            // FTP and FTPS
#include "./ssh/ssh.h"            // SSH
#include "./sftp/sftp.h"          // SFTP
#include "./smtp/smtp.h"          // SMTP
#include "./pop/pop.h"            // POP3
#include "./imap/imap.h"          // IMAP
#include "./curl/curl.h"          // libcurl's transfers


#endif  // DJINTERP_NET_NET_H
