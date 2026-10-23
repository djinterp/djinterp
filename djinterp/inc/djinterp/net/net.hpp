/*******************************************************************************
* djinterp [net]                                                         net.hpp
*
* The net subframework's C++ umbrella: including it includes every C++ face
* of net, with the foundation they share, net_common.hpp.
*   Each face's own header can be included alone instead; this one is for a
* program that wants them all. As net.h does for C, it includes headers whose
* backend, floor or language level may be absent: each of those compiles to
* nothing there, unless cfg_net.h's D_CFG_NET_REQUIRE_BACKENDS asks for an
* #error.
*
*
* path:      /inc/djinterp/net/net.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_NET_NET_HPP
#define DJINTERP_NET_NET_HPP 1

// djinterp
#include "./net_common.hpp"         // errors, endpoints, connections, streams
#include "./net_url.hpp"            // URLs, RFC 3986
#include "./tcp/tcp.hpp"            // TCP connections and listeners
#include "./tls/tls.hpp"            // TLS contexts and connections
#include "./http/http.hpp"          // HTTP: vocabulary, messages, headers
#include "./client.hpp"             // HTTP clients
#include "./server.hpp"             // HTTP servers
#include "./curl/curl.hpp"          // libcurl's transfers
#include "./ftp/ftp.hpp"            // FTP and FTPS
#include "./sftp/sftp.hpp"          // SFTP
#include "./smtp/smtp.hpp"          // SMTP
#include "./reactor.hpp"            // the event reactor
#include "./api/claude.hpp"         // the Claude API


#endif  // DJINTERP_NET_NET_HPP
