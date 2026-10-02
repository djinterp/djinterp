/*******************************************************************************
* djinterp [net]                                                   ssl_openssl.h
*
* The built-in OpenSSL-family engine.
*   Binds the SSL kernel of ssl.h to OpenSSL 1.1.0 or later, LibreSSL, or
* BoringSSL. It is compiled where env_ssl.h finds one of them and
* D_CFG_SSL_OPENSSL is on; otherwise d_ssl_engine_openssl returns NULL, so a
* caller asks for the engine without conditional compilation of its own.
*   The library runs sans-I/O, as the kernel's engine contract requires. Each
* session owns two memory BIOs: the kernel feeds one with ciphertext from the
* peer and drains the other toward the peer, and the library never sees a
* transport.
*
*
* path:      /inc/djinterp/net/ssl/ssl_openssl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ENGINE
    ------
    1.  OpenSSL
*/

#ifndef DJINTERP_NET_SSL_SSL_OPENSSL_H
#define DJINTERP_NET_SSL_SSL_OPENSSL_H 1

// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssl.h"             // d_ssl_engine


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENGINE
//==============================================================================
// The engine a build offers for the OpenSSL family, as the table the kernel's
// d_ssl_context_init takes. Its features follow the library found at compile
// time:
//     TLS1_3        where env_ssl.h reports D_ENV_SSL_OPENSSL_HAS_TLS1_3.
//     KEYLOG        where it reports D_ENV_SSL_OPENSSL_HAS_KEYLOG.
//     ALPN, MEMORY_PEM, CLIENT_AUTH, and SYSTEM_TRUST in every build.
// It declares neither HOSTNAME nor LEGACY: the kernel checks names itself,
// through peer_name, and refuses TLS 1.0 and 1.1 configurations with
// D_SSL_STATUS_UNSUPPORTED.
//   The system trust store is loaded in this order. When SSL_CERT_FILE or
// SSL_CERT_DIR is set, those variables alone decide it. Otherwise the
// library's default locations are loaded, then the first file of
// d_ssl_ca_file_candidates and the first directory of
// d_ssl_ca_dir_candidates that load, then, on Windows, the ROOT store.
// Apple platforms have no candidates and rely on the library's defaults.
//   An engine context is read-only once created, so sessions may be created
// from one context on several threads at once. Each session is used from one
// thread at a time, as the kernel requires.
//   A program using the engine links the library's ssl and crypto libraries,
// and on Windows crypt32 as well.


// 1.1    OpenSSL
//------------------------------------------------------------------------------
/**
 * @brief Returns the OpenSSL-family engine.
 *
 * @return the engine, or `NULL` if D_INTERNAL_SSL_OPENSSL is 0.
 */
D_NODISCARD const struct d_ssl_engine* d_ssl_engine_openssl(void);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSL_SSL_OPENSSL_H
