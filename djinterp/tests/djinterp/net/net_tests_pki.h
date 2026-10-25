/*******************************************************************************
* djinterp [net]                                                 net_tests_pki.h
*
* The certificates net's TLS suites are tested with, made for each run.
*   A CA; from it a server certificate for localhost and 127.0.0.1, a client
* certificate, and an expired server certificate; and a server certificate
* from a second CA no test trusts -- written as PEM files to a directory of
* their own, and removed with it. Needs an OpenSSL-family library, as
* env_tls.h finds one; without it, making reports false.
*
*
* path:      /tests/djinterp/net/net_tests_pki.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_PKI_H
#define DJINTERP_NET_NET_TESTS_PKI_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../inc/djinterp/env/net/tls/env_tls.h"  // D_ENV_TLS_HAS_OPENSSL


D_EXTERN_C_BEGIN


// d_tests_net_pki
//   struct: the files of one run: the directory, then each certificate and
// key in it.
struct d_tests_net_pki
{
    char directory[256];
    char ca[320];
    char server_cert[320];
    char server_key[320];
    char client_cert[320];
    char client_key[320];
    char expired_cert[320];
    char expired_key[320];
    char rogue_cert[320];
    char rogue_key[320];
};

// the PKI -- made in a fresh directory under TMPDIR, and removed with it;
// making reports false, leaving nothing behind, on any failure
bool d_tests_net_pki_make(struct d_tests_net_pki* _pki);
void d_tests_net_pki_remove(const struct d_tests_net_pki* _pki);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_PKI_H
