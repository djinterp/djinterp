// FTPS test support: what ftps_tests_support.c exports to the suites.
#ifndef DJINTERP_TESTS_NET_FTP_FTPS_TESTS_SUPPORT_H
#define DJINTERP_TESTS_NET_FTP_FTPS_TESTS_SUPPORT_H 1

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include "../../../../inc/djinterp/net/ftp/ftp.h"
#include "../../../../inc/djinterp/net/ssl/ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

// the toy engine, and what the suites may change about it
struct toy_settings { int client_cache; const char* cert_name; uint32_t chain_verdict; };
extern struct toy_settings      g_toy;
extern const struct d_ssl_engine TOY;

// the server: its behaviour, and what it saw
struct srv_cfg { int implicit, refuse_auth, inject, require_reuse, truncate, refuse_epsv, bogus_pasv, no_mlst; };
struct srv
{
    struct srv_cfg cfg; struct d_ssl_context tls; struct d_tcp_listener listener; uint16_t port; pthread_t thread;
    int saw_user_in_clear, logins, epsv_count, data_tls, data_plain, data_resumed, data_fresh, ccc_done, upload_clean, upload_waited;
    char type; char stored[4096]; size_t stored_len;
};


// the files it serves: hello.txt, and big.bin, 200000 bytes of g_big
extern char       g_big[200000];
extern const char HELLO[];

// srv_start listens on 127.0.0.1 and serves one session on a thread;
// srv_stop joins it
int  srv_start(struct srv* _server, struct srv_cfg _config);
void srv_stop(struct srv* _server);

#ifdef __cplusplus
}
#endif

#endif  // DJINTERP_TESTS_NET_FTP_FTPS_TESTS_SUPPORT_H
