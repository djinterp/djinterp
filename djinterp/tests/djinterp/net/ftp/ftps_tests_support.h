/*******************************************************************************
* djinterp [net]                                            ftps_tests_support.h
*
* FTPS test support: what ftps_tests_support.c gives the suites -- the toy
* TLS engine and its settings, the threaded server and what it saw, and the
* files it serves.
*
*
* path:      /tests/djinterp/net/ftp/ftps_tests_support.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_NET_FTP_FTPS_TESTS_SUPPORT_H
#define DJINTERP_NET_FTP_FTPS_TESTS_SUPPORT_H 1

// std
#include <stddef.h>  // size_t
// POSIX
#include <pthread.h>  // pthread_t
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp.h"  // d_ftp_*, d_tcp_listener
#include "../../../../inc/djinterp/net/ssl/ssl.h"  // d_ssl_context, engines
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint16_t, uint32_t


D_EXTERN_C_BEGIN

// d_tests_ftps_toy_settings
//   struct: what the suites may change about the toy engine.
struct d_tests_ftps_toy_settings
{
    int         client_cache;   // client contexts cache a session per host
    const char* cert_name;      // the name the server's certificate carries
    uint32_t    chain_verdict;  // the chain check's verdict; 0 accepts
};

// g_toy, TOY
//   variable: the toy engine's settings, and the engine itself.
extern struct d_tests_ftps_toy_settings g_toy;
extern const struct d_ssl_engine        TOY;

// d_tests_ftps_server_config
//   struct: how the server behaves; each field is 0 or 1.
struct d_tests_ftps_server_config
{
    int implicit;       // TLS from the first octet
    int refuse_auth;    // AUTH TLS is refused
    int inject;         // AUTH TLS's reply carries an injected login
    int require_reuse;  // a data connection must resume the control session
    int truncate;       // an upload's data connection ends with no close notify
    int refuse_epsv;    // EPSV is refused
    int bogus_pasv;     // PASV names 10.255.255.1, not the server
    int no_mlst;        // FEAT offers no MLST
};

// d_tests_ftps_server
//   struct: the server, its listener and thread, and what it saw.
struct d_tests_ftps_server
{
    struct d_tests_ftps_server_config cfg;           // its behaviour
    struct d_ssl_context              tls;           // its toy context
    struct d_tcp_listener             listener;      // on 127.0.0.1
    uint16_t                          port;          // the listener's port
    pthread_t                         thread;        // the session's thread
    int                               saw_user_in_clear;  // USER before TLS
    int                               logins;        // successful logins
    int                               epsv_count;    // EPSV commands
    int                               data_tls;      // secured data
    int                               data_plain;    // plain data
    int                               data_resumed;  // resumed data sessions
    int                               data_fresh;    // full data handshakes
    int                               ccc_done;      // CCC ended TLS cleanly
    int                               upload_clean;  // an upload's orderly end
    int                               upload_waited; // the client waited for it
    char                              type;          // the last TYPE
    char                              stored[4096];  // an upload's first octets
    size_t                            stored_len;    // how many it kept
};

// g_big, HELLO
//   variable: the files the server serves -- big.bin, 200000 octets of
// g_big, and hello.txt.
extern char       g_big[200000];
extern const char HELLO[];

/**
 * @brief Starts the server on an ephemeral port of 127.0.0.1, serving one
 *        session on its own thread.
 *
 * @param[out] _server  the server; its port and counters are set here.
 * @param[in]  _config  how it behaves.
 * @return 1 when it serves, 0 when it could not start.
 */
int  d_tests_ftps_server_start(struct d_tests_ftps_server*       _server,
                               struct d_tests_ftps_server_config _config);

/**
 * @brief Joins the server's session, then releases its listener and context.
 *
 * @param[in,out] _server  a started server.
 */
void d_tests_ftps_server_stop(struct d_tests_ftps_server* _server);

D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTPS_TESTS_SUPPORT_H
