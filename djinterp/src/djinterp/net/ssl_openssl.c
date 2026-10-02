/*******************************************************************************
* djinterp [net]                                                   ssl_openssl.c
*
*   Definitions for ssl_openssl.h: the OpenSSL-family engine, an adapter from
* the kernel's sans-I/O engine interface to libssl running over memory BIOs.
*   Five mechanisms carry it. Each memory BIO answers an empty read with
* "retry" rather than end-of-file, so a library waiting on the peer reports
* WANT_READ instead of a truncated stream. The thread's error queue is
* cleared before every library call whose outcome SSL_get_error reads, since
* a stale error left by an earlier call would otherwise decide it. An info
* callback records alerts as the library writes and reads them, and a failed
* step is classified by the alert this end sent -- a certificate alert is a
* verification failure, a negotiation alert a failed handshake -- which reads
* the same in every OpenSSL fork, where the library's reason codes do not. A
* write takes at most one record of plaintext, so no call leaves more than a
* record's ciphertext inside the engine. And the peer's names and
* certificate are cached per session, backing the borrowed results the
* kernel reads.
*
*
* path:      /src/djinterp/net/ssl/ssl_openssl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"  // corresponding header
// std
#include <limits.h>   // INT_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // int32_t, uint32_t
#include <stdlib.h>   // calloc, malloc, free, getenv
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"              // framework root
#include "../../../../inc/djinterp/config/net/ssl/cfg_ssl.h"  // D_INTERNAL_SSL_*
#include "../../../../inc/djinterp/env/net/env_ssl.h"         // D_ENV_SSL_*
#include "../../../../inc/djinterp/env/net/env_tls.h"         // D_ENV_TLS_*
#include "../../../../inc/djinterp/net/ssl/ssl.h"             // d_ssl_engine

#if ( (D_INTERNAL_SSL_OPENSSL == 1) &&                                         \
      (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_WINDOWS) )
    // windows: ahead of the library's headers, which undefine the names
    // wincrypt.h would otherwise take from them
    #include <windows.h>   // HCERTSTORE, PCCERT_CONTEXT
    #include <wincrypt.h>  // CertOpenSystemStoreW, CertCloseStore
#endif  // D_ENV_SSL_TRUST_STORE

#if (D_INTERNAL_SSL_OPENSSL == 1)
    // openssl
    #include <openssl/bio.h>     // BIO, BIO_s_mem, BIO_new_mem_buf
    #include <openssl/crypto.h>  // OPENSSL_free
    #include <openssl/err.h>     // ERR_clear_error, ERR_peek_last_error
    #include <openssl/evp.h>     // EVP_PKEY, EVP_PKEY_free
    #include <openssl/pem.h>     // PEM_read_bio_X509, PEM_read_bio_PrivateKey
    #include <openssl/ssl.h>     // SSL, SSL_CTX, SSL_CIPHER
    #include <openssl/x509.h>    // X509, X509_STORE, X509_V_*
    #include <openssl/x509v3.h>  // GENERAL_NAMES, NID_subject_alt_name
#endif  // D_INTERNAL_SSL_OPENSSL


#if (D_INTERNAL_SSL_OPENSSL == 1)

// the kernel's versions are the wire code points the library's version
// constants also are, so the engine passes one for the other
D_STATIC_ASSERT((TLS1_2_VERSION == D_SSL_VERSION_TLS1_2),
                "TLS 1.2 must share the kernel's code point");

#if (D_ENV_SSL_OPENSSL_HAS_TLS1_3 == 1)
D_STATIC_ASSERT((TLS1_3_VERSION == D_SSL_VERSION_TLS1_3),
                "TLS 1.3 must share the kernel's code point");
#endif  // D_ENV_SSL_OPENSSL_HAS_TLS1_3

// d_internal_ssl_openssl_context
//   struct: an engine context: the library's context, the plan it was made
// for, and what its callbacks read -- the ALPN list a server selects from,
// and the key-log sink.
struct d_internal_ssl_openssl_context
{
    SSL_CTX*          library;
    struct d_ssl_plan plan;
    unsigned char*    alpn;
    size_t            alpn_size;
    d_ssl_keylog_fn   keylog;
    void*             keylog_context;
};

// d_internal_ssl_openssl_session
//   struct: an engine session: the library's session, which owns both memory
// BIOs; the last alert each way, as the info callback saw them; and the
// caches behind the borrowed results of peer_name and peer_certificate.
struct d_internal_ssl_openssl_session
{
    const struct d_internal_ssl_openssl_context* context;
    SSL*                                         library;
    BIO*                                         inbound;
    BIO*                                         outbound;
    int32_t                                      alert_sent;
    int32_t                                      alert_received;
    GENERAL_NAMES*                               names;
    unsigned char*                               der;
    size_t                                       der_size;
};

#if (D_ENV_SSL_OPENSSL_HAS_GET1_PEER == 1)

/*
d_internal_ssl_openssl_peer
  Returns the peer's certificate as a new reference the caller frees, or NULL
when none was presented. OpenSSL 3.0 renamed the call to say so.
*/
static X509*
d_internal_ssl_openssl_peer(
    const SSL* _library
)
{
    return SSL_get1_peer_certificate(_library);
}

#else

/*
d_internal_ssl_openssl_peer
  Returns the peer's certificate as a new reference the caller frees, or NULL
when none was presented. Before OpenSSL 3.0, and in the forks, the call has
always returned a new reference under its older name.
*/
static X509*
d_internal_ssl_openssl_peer(
    const SSL* _library
)
{
    return SSL_get_peer_certificate(_library);
}

#endif  // D_ENV_SSL_OPENSSL_HAS_GET1_PEER

/*
d_internal_ssl_openssl_out_of_memory
  Whether the library's most recent error is an allocation failure: the one
failure an engine reports as OUT_OF_MEMORY rather than as a verdict on the
exchange.
*/
static bool
d_internal_ssl_openssl_out_of_memory(void)
{
    const unsigned long error = ERR_peek_last_error();

    return (ERR_GET_REASON(error) == ERR_R_MALLOC_FAILURE);
}

/*
d_internal_ssl_openssl_missing_certificate
  Whether the library failed because the peer withheld a certificate the
plan requires. The alert sent for that differs by version --
certificate_required in TLS 1.3, handshake_failure in TLS 1.2 -- so it is
recognized by the library's reason instead, which every fork shares.
*/
static bool
d_internal_ssl_openssl_missing_certificate(void)
{
    const unsigned long error = ERR_peek_error();

    return ( (ERR_GET_LIB(error) == ERR_LIB_SSL) &&
             (ERR_GET_REASON(error) ==
                  SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE) );
}

/*
d_internal_ssl_openssl_library_failure
  The status of a library call that failed outside any exchange -- creating a
context, a session, or a BIO -- clearing the error it left behind.
*/
static enum d_ssl_status
d_internal_ssl_openssl_library_failure(void)
{
    const bool exhausted = d_internal_ssl_openssl_out_of_memory();

    ERR_clear_error();

    return (exhausted) ? D_SSL_STATUS_OUT_OF_MEMORY
                       : D_SSL_STATUS_BACKEND_ERROR;
}

/*
d_internal_ssl_openssl_alert_status
  Classifies a failed exchange by the fatal alert this end sent. Certificate
alerts mean the peer failed verification, negotiation alerts that the two
ends found nothing to agree on, internal_error that the library itself
failed, and the rest that the peer broke the protocol.
*/
static enum d_ssl_status
d_internal_ssl_openssl_alert_status(
    int32_t _alert
)
{
    switch (_alert)
    {
        // the peer's certificate did not pass
        case D_SSL_ALERT_BAD_CERTIFICATE:
        case D_SSL_ALERT_UNSUPPORTED_CERTIFICATE:
        case D_SSL_ALERT_CERTIFICATE_REVOKED:
        case D_SSL_ALERT_CERTIFICATE_EXPIRED:
        case D_SSL_ALERT_CERTIFICATE_UNKNOWN:
        case D_SSL_ALERT_UNKNOWN_CA:
        case D_SSL_ALERT_CERTIFICATE_REQUIRED:
            return D_SSL_STATUS_VERIFY_FAILED;

        // no version, suite, extension, or protocol in common
        case D_SSL_ALERT_HANDSHAKE_FAILURE:
        case D_SSL_ALERT_ACCESS_DENIED:
        case D_SSL_ALERT_PROTOCOL_VERSION:
        case D_SSL_ALERT_INSUFFICIENT_SECURITY:
        case D_SSL_ALERT_INAPPROPRIATE_FALLBACK:
        case D_SSL_ALERT_MISSING_EXTENSION:
        case D_SSL_ALERT_UNSUPPORTED_EXTENSION:
        case D_SSL_ALERT_UNRECOGNIZED_NAME:
        case D_SSL_ALERT_NO_APPLICATION_PROTOCOL:
            return D_SSL_STATUS_HANDSHAKE_FAILED;

        // the library failed, not the exchange
        case D_SSL_ALERT_INTERNAL_ERROR:
            return D_SSL_STATUS_BACKEND_ERROR;

        // malformed or unexpected input from the peer
        default:
            return D_SSL_STATUS_PROTOCOL_ERROR;
    }
}

/*
d_internal_ssl_openssl_failure
  The status of a step that failed with SSL_ERROR_SSL. An alert this end sent
explains the failure best, except for a required certificate the peer
withheld, which is recognized first. An alert the peer sent means the peer
refused: the handshake, while it runs, and the protocol after. With neither,
the peer's input was malformed.
*/
static enum d_ssl_status
d_internal_ssl_openssl_failure(
    const struct d_internal_ssl_openssl_session* _session
)
{
    // running out of memory is the library's failure, not the exchange's
    if (d_internal_ssl_openssl_out_of_memory())
    {
        return D_SSL_STATUS_OUT_OF_MEMORY;
    }

    // a required certificate never arrived: the peer failed verification
    if (d_internal_ssl_openssl_missing_certificate())
    {
        return D_SSL_STATUS_VERIFY_FAILED;
    }

    // a fatal alert this end sent names what it found wrong
    if ( (_session->alert_sent != D_SSL_ALERT_NONE)         &&
         (_session->alert_sent != D_SSL_ALERT_CLOSE_NOTIFY) )
    {
        return d_internal_ssl_openssl_alert_status(_session->alert_sent);
    }

    // a fatal alert from the peer is its refusal
    if ( (_session->alert_received != D_SSL_ALERT_NONE)         &&
         (_session->alert_received != D_SSL_ALERT_CLOSE_NOTIFY) )
    {
        return (SSL_in_init(_session->library) == 1)
                   ? D_SSL_STATUS_HANDSHAKE_FAILED
                   : D_SSL_STATUS_PROTOCOL_ERROR;
    }

    return D_SSL_STATUS_PROTOCOL_ERROR;
}

/*
d_internal_ssl_openssl_outcome
  Translates the failure of a handshake, read, write, or shutdown call into
the engine's statuses. SSL_ERROR_WANT_WRITE cannot arise, since a memory BIO
takes every write, and with the other remaining errors it is the backend's.
*/
static enum d_ssl_status
d_internal_ssl_openssl_outcome(
    const struct d_internal_ssl_openssl_session* _session,
    int                                          _result
)
{
    const int error = SSL_get_error(_session->library,
                                    _result);

    switch (error)
    {
        // the library waits on the peer
        case SSL_ERROR_WANT_READ:
            return D_SSL_STATUS_WANT_READ;

        // the peer's close_notify arrived
        case SSL_ERROR_ZERO_RETURN:
            return D_SSL_STATUS_CONNECTION_CLOSED;

        // the exchange failed
        case SSL_ERROR_SSL:
            return d_internal_ssl_openssl_failure(_session);

        default:
            return D_SSL_STATUS_BACKEND_ERROR;
    }
}

/*
d_internal_ssl_openssl_on_info
  The library's state callback, used for alerts alone. It sees each alert as
it is written or read, so the last one each way is what describe reports and
what d_internal_ssl_openssl_failure reads.
*/
static void
d_internal_ssl_openssl_on_info(
    const SSL* _library,
    int        _where,
    int        _value
)
{
    // only alerts are recorded
    if ((_where & SSL_CB_ALERT) == 0)
    {
        return;
    }

    struct d_internal_ssl_openssl_session* const session =
        SSL_get_app_data(_library);

    // a session not yet attached has nowhere to record them
    if (session == NULL)
    {
        return;
    }

    // the low byte of the value is the alert's description
    const int32_t alert = (int32_t)(_value & 0xFF);

    // written by this end, or read from the peer
    if ((_where & SSL_CB_WRITE) != 0)
    {
        session->alert_sent = alert;
    }
    else
    {
        session->alert_received = alert;
    }

    return;
}

/*
d_internal_ssl_openssl_on_verify_any
  The verification callback of a context whose plan requests certificates
but trusts pins alone: every chain passes here, and the kernel checks the
pins once the handshake completes.
*/
static int
d_internal_ssl_openssl_on_verify_any(
    int             _verdict,
    X509_STORE_CTX* _store
)
{
    (void)_verdict;
    (void)_store;

    return 1;
}

/*
d_internal_ssl_openssl_on_alpn
  A server's ALPN selection: the first protocol on the server's list that the
client also offered. With none in common the extension goes unanswered,
unless the plan requires a protocol, when the handshake fails with a
no_application_protocol alert. The parameter list is libssl's, which is why
it runs past five.
*/
static int
d_internal_ssl_openssl_on_alpn(
    SSL*                  _library,
    const unsigned char** _out,
    unsigned char*        _out_size,
    const unsigned char*  _offered,
    unsigned int          _offered_size,
    void*                 _context
)
{
    const struct d_internal_ssl_openssl_context* const context = _context;

    (void)_library;

    const int agreed = SSL_select_next_proto((unsigned char**)_out,
                                             _out_size,
                                             context->alpn,
                                             (unsigned int)context->alpn_size,
                                             _offered,
                                             _offered_size);

    // a protocol both sides name is the answer
    if (agreed == OPENSSL_NPN_NEGOTIATED)
    {
        return SSL_TLSEXT_ERR_OK;
    }

    return (context->plan.require_alpn) ? SSL_TLSEXT_ERR_ALERT_FATAL
                                        : SSL_TLSEXT_ERR_NOACK;
}

#if (D_ENV_SSL_OPENSSL_HAS_KEYLOG == 1)

/*
d_internal_ssl_openssl_on_keylog
  Hands one NSS key-log line to the configuration's sink, found through the
library context the engine context is attached to.
*/
static void
d_internal_ssl_openssl_on_keylog(
    const SSL*  _library,
    const char* _line
)
{
    const struct d_internal_ssl_openssl_context* const context =
        SSL_CTX_get_app_data(SSL_get_SSL_CTX(_library));

    // the callback is installed only with a sink; this guards the unexpected
    if ( (context == NULL)         ||
         (context->keylog == NULL) )
    {
        return;
    }

    context->keylog(context->keylog_context,
                    _line);

    return;
}

#endif  // D_ENV_SSL_OPENSSL_HAS_KEYLOG

/*
d_internal_ssl_openssl_text_bio
  A read-only memory BIO over caller text, which it borrows rather than
copies, so no private key is duplicated here. NULL for text longer than the
library's int lengths allow, or when the BIO cannot be allocated.
*/
static BIO*
d_internal_ssl_openssl_text_bio(
    struct d_pack_text _text
)
{
    // the library measures buffers in int
    if (_text.length > (size_t)INT_MAX)
    {
        return NULL;
    }

    return BIO_new_mem_buf(_text.data,
                           (int)_text.length);
}

/*
d_internal_ssl_openssl_set_versions
  Fixes the context's version window to the plan's, passing the kernel's
versions straight through as the code points the static assertions above
pin. The library refuses a version it was built without.
*/
static enum d_ssl_status
d_internal_ssl_openssl_set_versions(
    SSL_CTX*                 _library,
    const struct d_ssl_plan* _plan
)
{
    const long lowest  = SSL_CTX_set_min_proto_version(
                             _library,
                             (uint16_t)_plan->min_version);
    const long highest = SSL_CTX_set_max_proto_version(
                             _library,
                             (uint16_t)_plan->max_version);

    // either end of the window refused
    if ( (lowest != 1)  ||
         (highest != 1) )
    {
        ERR_clear_error();

        return D_SSL_STATUS_UNSUPPORTED;
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_set_verification
  Maps the plan's certificate decisions onto the library's verify mode. A
client checks the server's chain when the plan says so, and otherwise only
receives it, for the kernel to check pins against. A server asks for a
certificate when the plan requests one and insists when it requires one. A
plan that asks for certificates but trusts pins alone passes every chain
here, leaving the pins to the kernel.
*/
static void
d_internal_ssl_openssl_set_verification(
    SSL_CTX*                 _library,
    const struct d_ssl_plan* _plan
)
{
    const bool asks    = ( (_plan->verify_chain) ||
                           (_plan->request_certificate) );
    const bool insists = ( (_plan->role == D_SSL_ROLE_SERVER) &&
                           (_plan->require_certificate) );
    const int  mode    = ( ((asks) ? SSL_VERIFY_PEER : SSL_VERIFY_NONE) |
                           ((insists) ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0) );
    int (* const check)(int, X509_STORE_CTX*) =
        (_plan->verify_chain) ? NULL
                              : d_internal_ssl_openssl_on_verify_any;

    SSL_CTX_set_verify(_library,
                       mode,
                       check);

    return;
}

/*
d_internal_ssl_openssl_load_pem_anchors
  Adds every certificate in PEM text to the context's trust store. Text that
yields no certificate at all is INVALID_CONFIG. The library's "no further
PEM" error, which ends every such loop, is cleared.
*/
static enum d_ssl_status
d_internal_ssl_openssl_load_pem_anchors(
    SSL_CTX*           _library,
    struct d_pack_text _pem
)
{
    BIO* const source = d_internal_ssl_openssl_text_bio(_pem);

    // text too long for the library, or no memory for the BIO
    if (source == NULL)
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    X509_STORE* const store       = SSL_CTX_get_cert_store(_library);
    size_t            added       = 0;
    X509*             certificate = PEM_read_bio_X509(source,
                                                      NULL,
                                                      NULL,
                                                      NULL);

    // each certificate becomes an anchor; the store keeps its own reference
    while (certificate != NULL)
    {
        if (X509_STORE_add_cert(store,
                                certificate) == 1)
        {
            added++;
        }

        X509_free(certificate);
        certificate = PEM_read_bio_X509(source,
                                        NULL,
                                        NULL,
                                        NULL);
    }

    BIO_free(source);
    ERR_clear_error();

    return (added > 0) ? D_SSL_STATUS_OK
                       : D_SSL_STATUS_INVALID_CONFIG;
}

/*
d_internal_ssl_openssl_load_candidates
  Loads the first CA bundle file, and the first certificate directory, among
the kernel's platform candidates that the library can read. The failures the
others leave are cleared: a candidate that does not exist is not an error.
*/
static void
d_internal_ssl_openssl_load_candidates(
    SSL_CTX* _library
)
{
    size_t                   file_count = 0;
    const char* const* const files      = d_ssl_ca_file_candidates(&file_count);

    // the first bundle that loads
    for (size_t i = 0; i < file_count; i++)
    {
        if (SSL_CTX_load_verify_locations(_library,
                                          files[i],
                                          NULL) == 1)
        {
            break;
        }
    }

    size_t                   dir_count = 0;
    const char* const* const dirs      = d_ssl_ca_dir_candidates(&dir_count);

    // the first directory that loads
    for (size_t i = 0; i < dir_count; i++)
    {
        if (SSL_CTX_load_verify_locations(_library,
                                          NULL,
                                          dirs[i]) == 1)
        {
            break;
        }
    }

    ERR_clear_error();

    return;
}

#if (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_WINDOWS)

/*
d_internal_ssl_openssl_load_platform
  Adds the current user's ROOT store to the context's trust store. The
library does not read Windows' stores itself, so each certificate is decoded
from its DER encoding; one that does not decode is skipped. A store that
cannot be opened adds nothing, and chains then fail as UNTRUSTED.
*/
static void
d_internal_ssl_openssl_load_platform(
    SSL_CTX* _library
)
{
    X509_STORE* const store  = SSL_CTX_get_cert_store(_library);
    HCERTSTORE const  system = CertOpenSystemStoreW(0,
                                                    L"ROOT");

    // no store to read
    if (system == NULL)
    {
        return;
    }

    PCCERT_CONTEXT entry = CertEnumCertificatesInStore(system,
                                                       NULL);

    // each entry in turn; the enumeration frees the one it moves past
    while (entry != NULL)
    {
        const unsigned char* der         = entry->pbCertEncoded;
        X509* const          certificate = d2i_X509(NULL,
                                                    &der,
                                                    (long)entry->cbCertEncoded);

        if (certificate != NULL)
        {
            (void)X509_STORE_add_cert(store,
                                      certificate);
            X509_free(certificate);
        }

        entry = CertEnumCertificatesInStore(system,
                                            entry);
    }

    (void)CertCloseStore(system,
                         0);

    return;
}

#else

/*
d_internal_ssl_openssl_load_platform
  Elsewhere the platform's anchors are files, which the library's defaults
and the kernel's candidates have already covered.
*/
static void
d_internal_ssl_openssl_load_platform(
    SSL_CTX* _library
)
{
    (void)_library;

    return;
}

#endif  // D_ENV_SSL_TRUST_STORE

/*
d_internal_ssl_openssl_load_system
  Loads the platform's trust anchors in the order ssl_openssl.h documents.
The library's default locations honour SSL_CERT_FILE and SSL_CERT_DIR, so
when either is set they are loaded alone. Loading is best effort: a chain
nothing anchors fails verification later, as UNTRUSTED.
*/
static void
d_internal_ssl_openssl_load_system(
    SSL_CTX* _library
)
{
    const char* const file       = getenv("SSL_CERT_FILE");
    const char* const directory  = getenv("SSL_CERT_DIR");
    const bool        overridden = ( ( (file != NULL) &&
                                       (file[0] != '\0') )      ||
                                     ( (directory != NULL) &&
                                       (directory[0] != '\0') ) );

    (void)SSL_CTX_set_default_verify_paths(_library);

    // without an override the platform's own locations follow
    if (!overridden)
    {
        d_internal_ssl_openssl_load_candidates(_library);
        d_internal_ssl_openssl_load_platform(_library);
    }

    ERR_clear_error();

    return;
}

/*
d_internal_ssl_openssl_load_anchors
  Loads every trust anchor the configuration names: PEM text, a bundle file,
a hashed directory, and, where the plan checks chains, the platform's store.
A named source that does not load is INVALID_CONFIG.
*/
static enum d_ssl_status
d_internal_ssl_openssl_load_anchors(
    SSL_CTX*                   _library,
    const struct d_ssl_config* _config,
    const struct d_ssl_plan*   _plan
)
{
    // anchors given as text
    if (_config->ca_pem.length > 0)
    {
        const enum d_ssl_status status =
            d_internal_ssl_openssl_load_pem_anchors(_library,
                                                    _config->ca_pem);

        if (status != D_SSL_STATUS_OK)
        {
            return status;
        }
    }

    // a bundle file, a hashed directory, or both
    if ( (_config->ca_file != NULL) ||
         (_config->ca_path != NULL) )
    {
        const int loaded = SSL_CTX_load_verify_locations(_library,
                                                         _config->ca_file,
                                                         _config->ca_path);

        if (loaded != 1)
        {
            ERR_clear_error();

            return D_SSL_STATUS_INVALID_CONFIG;
        }
    }

    // the platform's store, only where a chain will be checked against it
    if ( (_config->use_system_trust) &&
         (_plan->verify_chain) )
    {
        d_internal_ssl_openssl_load_system(_library);
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_use_pem_chain
  Installs a certificate chain given as PEM text, leaf first: the leaf as the
context's certificate and each later certificate as a link of its chain.
*/
static enum d_ssl_status
d_internal_ssl_openssl_use_pem_chain(
    SSL_CTX*           _library,
    struct d_pack_text _pem
)
{
    BIO* const source = d_internal_ssl_openssl_text_bio(_pem);

    // text too long for the library, or no memory for the BIO
    if (source == NULL)
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    X509* const       leaf   = PEM_read_bio_X509(source,
                                                 NULL,
                                                 NULL,
                                                 NULL);
    enum d_ssl_status status = D_SSL_STATUS_INVALID_CONFIG;

    // the first certificate is the context's own
    if ( (leaf != NULL) &&
         (SSL_CTX_use_certificate(_library,
                                  leaf) == 1) )
    {
        status = D_SSL_STATUS_OK;
    }

    X509_free(leaf);

    // each later certificate extends the chain
    while (status == D_SSL_STATUS_OK)
    {
        X509* const extra = PEM_read_bio_X509(source,
                                              NULL,
                                              NULL,
                                              NULL);

        // the text ends where no further certificate parses
        if (extra == NULL)
        {
            break;
        }

        const long added = SSL_CTX_add1_chain_cert(_library,
                                                   extra);

        X509_free(extra);
        status = (added == 1) ? D_SSL_STATUS_OK
                              : D_SSL_STATUS_INVALID_CONFIG;
    }

    BIO_free(source);
    ERR_clear_error();

    return status;
}

/*
d_internal_ssl_openssl_use_pem_key
  Installs a private key given as PEM text. The text is read in place, and
the parsed key is released once the context holds its own reference.
*/
static enum d_ssl_status
d_internal_ssl_openssl_use_pem_key(
    SSL_CTX*           _library,
    struct d_pack_text _pem
)
{
    BIO* const source = d_internal_ssl_openssl_text_bio(_pem);

    // text too long for the library, or no memory for the BIO
    if (source == NULL)
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    EVP_PKEY* const   key    = PEM_read_bio_PrivateKey(source,
                                                       NULL,
                                                       NULL,
                                                       NULL);
    enum d_ssl_status status = D_SSL_STATUS_INVALID_CONFIG;

    // a key that parses and that the library accepts
    if ( (key != NULL) &&
         (SSL_CTX_use_PrivateKey(_library,
                                 key) == 1) )
    {
        status = D_SSL_STATUS_OK;
    }

    EVP_PKEY_free(key);
    BIO_free(source);
    ERR_clear_error();

    return status;
}

/*
d_internal_ssl_openssl_use_identity
  Installs the local identity, if the configuration has one: a certificate
chain and its private key, each from a file or from PEM text independently,
and then checks that the key belongs to the certificate.
*/
static enum d_ssl_status
d_internal_ssl_openssl_use_identity(
    SSL_CTX*                   _library,
    const struct d_ssl_config* _config
)
{
    // no identity: nothing to install or check
    if ( (_config->certificate_file == NULL) &&
         (_config->certificate_pem.length == 0) )
    {
        return D_SSL_STATUS_OK;
    }

    enum d_ssl_status status = D_SSL_STATUS_INVALID_CONFIG;

    // the chain, from its file or its text
    if (_config->certificate_file != NULL)
    {
        status = (SSL_CTX_use_certificate_chain_file(
                      _library,
                      _config->certificate_file) == 1)
                     ? D_SSL_STATUS_OK
                     : D_SSL_STATUS_INVALID_CONFIG;
    }
    else
    {
        status = d_internal_ssl_openssl_use_pem_chain(
                     _library,
                     _config->certificate_pem);
    }

    // the key, likewise
    if ( (status == D_SSL_STATUS_OK) &&
         (_config->private_key_file != NULL) )
    {
        status = (SSL_CTX_use_PrivateKey_file(_library,
                                              _config->private_key_file,
                                              SSL_FILETYPE_PEM) == 1)
                     ? D_SSL_STATUS_OK
                     : D_SSL_STATUS_INVALID_CONFIG;
    }
    else if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_openssl_use_pem_key(
                     _library,
                     _config->private_key_pem);
    }

    // the key must belong to the certificate
    if ( (status == D_SSL_STATUS_OK) &&
         (SSL_CTX_check_private_key(_library) != 1) )
    {
        status = D_SSL_STATUS_INVALID_CONFIG;
    }

    ERR_clear_error();

    return status;
}

#if (D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES == 1)

/*
d_internal_ssl_openssl_set_suites
  Sets the TLS 1.3 cipher suites, in the library's own syntax. The library
refuses a list naming no suite it knows.
*/
static enum d_ssl_status
d_internal_ssl_openssl_set_suites(
    SSL_CTX*    _library,
    const char* _suites
)
{
    // the library refuses a list that selects nothing
    if (SSL_CTX_set_ciphersuites(_library,
                                 _suites) != 1)
    {
        ERR_clear_error();

        return D_SSL_STATUS_INVALID_CONFIG;
    }

    return D_SSL_STATUS_OK;
}

#else

/*
d_internal_ssl_openssl_set_suites
  A library without SSL_CTX_set_ciphersuites cannot honour a TLS 1.3 suite
list, and a context that ignored one would do less than was asked.
*/
static enum d_ssl_status
d_internal_ssl_openssl_set_suites(
    SSL_CTX*    _library,
    const char* _suites
)
{
    (void)_library;
    (void)_suites;

    return D_SSL_STATUS_UNSUPPORTED;
}

#endif  // D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES

/*
d_internal_ssl_openssl_set_ciphers
  Applies the configuration's cipher strings: cipher_list for TLS 1.2 and
older, cipher_suites for TLS 1.3. Either left NULL keeps the library's
defaults.
*/
static enum d_ssl_status
d_internal_ssl_openssl_set_ciphers(
    SSL_CTX*                   _library,
    const struct d_ssl_config* _config
)
{
    // the library refuses a list that selects nothing
    if ( (_config->cipher_list != NULL) &&
         (SSL_CTX_set_cipher_list(_library,
                                  _config->cipher_list) != 1) )
    {
        ERR_clear_error();

        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // the TLS 1.3 suites, where the configuration names them
    if (_config->cipher_suites != NULL)
    {
        return d_internal_ssl_openssl_set_suites(_library,
                                                 _config->cipher_suites);
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_set_alpn
  Installs the configuration's ALPN wire list. A client offers the list as it
stands. A server keeps its own copy for the selection callback, since the
configuration does not outlive context creation.
*/
static enum d_ssl_status
d_internal_ssl_openssl_set_alpn(
    struct d_internal_ssl_openssl_context* _context,
    const struct d_ssl_config*             _config
)
{
    const struct d_pack_bytes list = _config->alpn;

    // no list: nothing to offer or select from
    if (list.size == 0)
    {
        return D_SSL_STATUS_OK;
    }

    // a client offers it; this call alone answers 0 for success
    if (_context->plan.role == D_SSL_ROLE_CLIENT)
    {
        const int refused = SSL_CTX_set_alpn_protos(_context->library,
                                                    list.data,
                                                    (unsigned int)list.size);

        return (refused == 0) ? D_SSL_STATUS_OK
                              : d_internal_ssl_openssl_library_failure();
    }

    _context->alpn = malloc(list.size);

    // a server's copy
    if (_context->alpn == NULL)
    {
        return D_SSL_STATUS_OUT_OF_MEMORY;
    }

    memcpy(_context->alpn,
           list.data,
           list.size);
    _context->alpn_size = list.size;
    SSL_CTX_set_alpn_select_cb(_context->library,
                               d_internal_ssl_openssl_on_alpn,
                               _context);

    return D_SSL_STATUS_OK;
}

#if (D_ENV_SSL_OPENSSL_HAS_KEYLOG == 1)

/*
d_internal_ssl_openssl_set_keylog
  Connects the configuration's key-log sink, if it has one, to the library's
key-log callback.
*/
static void
d_internal_ssl_openssl_set_keylog(
    struct d_internal_ssl_openssl_context* _context,
    const struct d_ssl_config*             _config
)
{
    // only a configuration with a sink gets the callback
    if (_config->keylog == NULL)
    {
        return;
    }

    _context->keylog         = _config->keylog;
    _context->keylog_context = _config->keylog_context;
    SSL_CTX_set_keylog_callback(_context->library,
                                d_internal_ssl_openssl_on_keylog);

    return;
}

#else

/*
d_internal_ssl_openssl_set_keylog
  Without a key-log callback in the library the engine does not declare
KEYLOG, and the kernel refuses any configuration with a sink before it
arrives here.
*/
static void
d_internal_ssl_openssl_set_keylog(
    struct d_internal_ssl_openssl_context* _context,
    const struct d_ssl_config*             _config
)
{
    (void)_context;
    (void)_config;

    return;
}

#endif  // D_ENV_SSL_OPENSSL_HAS_KEYLOG

/*
d_internal_ssl_openssl_configure
  Applies a configuration and its plan to a fresh library context, one step
at a time; the first step to fail decides the status.
*/
static enum d_ssl_status
d_internal_ssl_openssl_configure(
    struct d_internal_ssl_openssl_context* _context,
    const struct d_ssl_config*             _config
)
{
    SSL_CTX* const    library = _context->library;
    enum d_ssl_status status  = d_internal_ssl_openssl_set_versions(
                                    library,
                                    &_context->plan);

    d_internal_ssl_openssl_set_verification(library,
                                            &_context->plan);

    // trust anchors
    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_openssl_load_anchors(library,
                                                     _config,
                                                     &_context->plan);
    }

    // the local identity
    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_openssl_use_identity(library,
                                                     _config);
    }

    // cipher strings
    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_openssl_set_ciphers(library,
                                                    _config);
    }

    // application protocols
    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_openssl_set_alpn(_context,
                                                 _config);
    }

    // callbacks find the engine context through the library's
    (void)SSL_CTX_set_app_data(library,
                               _context);
    (void)SSL_CTX_set_mode(library,
                           SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    d_internal_ssl_openssl_set_keylog(_context,
                                      _config);

    return status;
}

/*
d_internal_ssl_openssl_context_destroy
  Frees an engine context, the library context it holds, and a server's copy
of its ALPN list.
*/
static void
d_internal_ssl_openssl_context_destroy(
    void* _context
)
{
    struct d_internal_ssl_openssl_context* const context = _context;

    // nothing to free
    if (context == NULL)
    {
        return;
    }

    SSL_CTX_free(context->library);
    free(context->alpn);
    free(context);

    return;
}

/*
d_internal_ssl_openssl_context_create
  Creates an engine context: the library context for the plan's role,
configured by d_internal_ssl_openssl_configure. A context that cannot honour
its configuration is destroyed before the failure returns.
*/
static enum d_ssl_status
d_internal_ssl_openssl_context_create(
    const struct d_ssl_config* _config,
    const struct d_ssl_plan*   _plan,
    void**                     _out_context
)
{
    // parameter validation
    if ( (_config == NULL) ||
         (_plan == NULL)   ||
         (_out_context == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    struct d_internal_ssl_openssl_context* const context =
        calloc(1,
               sizeof(*context));

    // no memory for the engine's record
    if (context == NULL)
    {
        return D_SSL_STATUS_OUT_OF_MEMORY;
    }

    context->plan    = *_plan;
    context->library = SSL_CTX_new((_plan->role == D_SSL_ROLE_SERVER)
                                       ? TLS_server_method()
                                       : TLS_client_method());

    // no library context
    if (context->library == NULL)
    {
        free(context);

        return d_internal_ssl_openssl_library_failure();
    }

    const enum d_ssl_status status =
        d_internal_ssl_openssl_configure(context,
                                         _config);

    // a context that cannot honour its configuration is not kept
    if (status != D_SSL_STATUS_OK)
    {
        d_internal_ssl_openssl_context_destroy(context);

        return status;
    }

    *_out_context = context;

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_session_attach
  Gives a session its library session and the two memory BIOs it runs over.
Each BIO answers an empty read with "retry" rather than end-of-file: the
inbound one so that a library waiting on the peer reports WANT_READ, the
outbound one so that it never reports an end drain did not cause.
*/
static enum d_ssl_status
d_internal_ssl_openssl_session_attach(
    struct d_internal_ssl_openssl_session* _session,
    const char*                            _sni
)
{
    SSL* const library  = SSL_new(_session->context->library);
    BIO* const inbound  = BIO_new(BIO_s_mem());
    BIO* const outbound = BIO_new(BIO_s_mem());

    // all three, or none
    if ( (library == NULL) ||
         (inbound == NULL) ||
         (outbound == NULL) )
    {
        SSL_free(library);
        BIO_free(inbound);
        BIO_free(outbound);

        return d_internal_ssl_openssl_library_failure();
    }

    (void)BIO_set_mem_eof_return(inbound,
                                 -1);
    (void)BIO_set_mem_eof_return(outbound,
                                 -1);
    SSL_set_bio(library,
                inbound,
                outbound);
    (void)SSL_set_app_data(library,
                           _session);
    SSL_set_info_callback(library,
                          d_internal_ssl_openssl_on_info);
    _session->library  = library;
    _session->inbound  = inbound;
    _session->outbound = outbound;

    // a server waits for the client's hello
    if (_session->context->plan.role == D_SSL_ROLE_SERVER)
    {
        SSL_set_accept_state(library);

        return D_SSL_STATUS_OK;
    }

    SSL_set_connect_state(library);

    // a client names the server it wants, when the kernel gives a name
    if ( (_sni != NULL) &&
         (SSL_set_tlsext_host_name(library,
                                   _sni) != 1) )
    {
        return d_internal_ssl_openssl_library_failure();
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_session_destroy
  Frees a session in whatever state it was left: the library session, which
frees both BIOs with itself, and the caches behind borrowed results.
*/
static void
d_internal_ssl_openssl_session_destroy(
    void* _session
)
{
    struct d_internal_ssl_openssl_session* const session = _session;

    // nothing to free
    if (session == NULL)
    {
        return;
    }

    SSL_free(session->library);
    GENERAL_NAMES_free(session->names);
    OPENSSL_free(session->der);
    free(session);

    return;
}

/*
d_internal_ssl_openssl_session_create
  Creates a session in a context. The reference identity is not kept: names
are the kernel's to check, through peer_name. The server name, if any, goes
into the client's hello.
*/
static enum d_ssl_status
d_internal_ssl_openssl_session_create(
    void*       _context,
    const char* _host,
    const char* _sni,
    void**      _out_session
)
{
    // parameter validation
    if ( (_context == NULL) ||
         (_out_session == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    (void)_host;

    struct d_internal_ssl_openssl_session* const session =
        calloc(1,
               sizeof(*session));

    // no memory for the engine's record
    if (session == NULL)
    {
        return D_SSL_STATUS_OUT_OF_MEMORY;
    }

    session->context        = _context;
    session->alert_sent     = D_SSL_ALERT_NONE;
    session->alert_received = D_SSL_ALERT_NONE;

    const enum d_ssl_status status =
        d_internal_ssl_openssl_session_attach(session,
                                              _sni);

    // a session the library could not set up is not kept
    if (status != D_SSL_STATUS_OK)
    {
        d_internal_ssl_openssl_session_destroy(session);

        return status;
    }

    *_out_session = session;

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_feed
  Appends ciphertext from the peer to the inbound BIO, in pieces the
library's int lengths can express. A memory BIO refuses a write only when it
cannot grow.
*/
static enum d_ssl_status
d_internal_ssl_openssl_feed(
    void*       _session,
    const void* _data,
    size_t      _size
)
{
    // parameter validation
    if ( (_session == NULL) ||
         ( (_data == NULL) &&
           (_size > 0) ) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;
    const unsigned char* const                         bytes   = _data;
    size_t                                             taken   = 0;

    // everything, however many pieces it takes
    while (taken < _size)
    {
        const size_t remaining = _size - taken;
        const int    piece     = (remaining > (size_t)INT_MAX)
                                     ? INT_MAX
                                     : (int)remaining;
        const int    written   = BIO_write(session->inbound,
                                           bytes + taken,
                                           piece);

        if (written <= 0)
        {
            return D_SSL_STATUS_OUT_OF_MEMORY;
        }

        taken += (size_t)written;
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_drain
  Moves pending ciphertext out of the outbound BIO, as much as the buffer
holds and one int-sized piece at most; the kernel calls again for the rest.
*/
static enum d_ssl_status
d_internal_ssl_openssl_drain(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_size
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_buffer == NULL)  ||
         (_out_size == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;
    const size_t pending = BIO_ctrl_pending(session->outbound);
    const size_t wanted  = (pending < _capacity) ? pending : _capacity;
    const size_t count   = (wanted > (size_t)INT_MAX) ? (size_t)INT_MAX
                                                      : wanted;

    *_out_size = 0;

    // nothing pending, or nowhere to put it
    if (count == 0)
    {
        return D_SSL_STATUS_OK;
    }

    const int moved = BIO_read(session->outbound,
                               _buffer,
                               (int)count);

    // a memory BIO holding bytes always yields some
    if (moved <= 0)
    {
        return D_SSL_STATUS_BACKEND_ERROR;
    }

    *_out_size = (size_t)moved;

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_openssl_handshake
  Advances the handshake as far as the ciphertext fed so far allows. The
library answers 1 once the handshake is complete, on that call and on every
later one.
*/
static enum d_ssl_status
d_internal_ssl_openssl_handshake(
    void* _session
)
{
    // parameter validation
    if (_session == NULL)
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;

    ERR_clear_error();

    const int result = SSL_do_handshake(session->library);

    // complete, now or earlier
    if (result == 1)
    {
        return D_SSL_STATUS_OK;
    }

    return d_internal_ssl_openssl_outcome(session,
                                          result);
}

/*
d_internal_ssl_openssl_read
  Decrypts application data. The library absorbs post-handshake messages
here -- session tickets, key updates -- and answers WANT_READ when they were
all the fed ciphertext held.
*/
static enum d_ssl_status
d_internal_ssl_openssl_read(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_buffer == NULL)  ||
         (_capacity == 0)   ||
         (_out_read == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;
    const size_t limit = (_capacity > (size_t)INT_MAX) ? (size_t)INT_MAX
                                                       : _capacity;

    *_out_read = 0;
    ERR_clear_error();

    const int result = SSL_read(session->library,
                                _buffer,
                                (int)limit);

    // plaintext arrived
    if (result > 0)
    {
        *_out_read = (size_t)result;

        return D_SSL_STATUS_OK;
    }

    return d_internal_ssl_openssl_outcome(session,
                                          result);
}

/*
d_internal_ssl_openssl_write
  Encrypts at most one record's worth of application data, so the ciphertext
a call leaves inside the engine stays bounded; the kernel calls again for the
rest.
*/
static enum d_ssl_status
d_internal_ssl_openssl_write(
    void*       _session,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_data == NULL)    ||
         (_size == 0)       ||
         (_out_written == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;
    const size_t limit = (_size > (size_t)D_SSL_RECORD_PLAINTEXT_MAX)
                             ? (size_t)D_SSL_RECORD_PLAINTEXT_MAX
                             : _size;

    *_out_written = 0;
    ERR_clear_error();

    const int result = SSL_write(session->library,
                                 _data,
                                 (int)limit);

    // taken, all of it
    if (result > 0)
    {
        *_out_written = (size_t)result;

        return D_SSL_STATUS_OK;
    }

    return d_internal_ssl_openssl_outcome(session,
                                          result);
}

/*
d_internal_ssl_openssl_close
  Queues close_notify. The library answers 0 once it has sent its own and 1
once the peer's has arrived as well; either is success. A second call finds
close_notify already sent and does nothing.
*/
static enum d_ssl_status
d_internal_ssl_openssl_close(
    void* _session
)
{
    // parameter validation
    if (_session == NULL)
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;

    // close_notify goes out once
    if ((SSL_get_shutdown(session->library) & SSL_SENT_SHUTDOWN) != 0)
    {
        return D_SSL_STATUS_OK;
    }

    ERR_clear_error();

    const int result = SSL_shutdown(session->library);

    // sent, whether or not the peer's has arrived
    if (result >= 0)
    {
        return D_SSL_STATUS_OK;
    }

    return d_internal_ssl_openssl_outcome(session,
                                          result);
}

/*
d_internal_ssl_openssl_copy_text
  Copies NUL-terminated text into a fixed field, truncated to fit and always
terminated. NULL text leaves the field as it was.
*/
static void
d_internal_ssl_openssl_copy_text(
    char*       _field,
    size_t      _capacity,
    const char* _text
)
{
    // nothing to copy, or nowhere to put it
    if ( (_text == NULL) ||
         (_capacity == 0) )
    {
        return;
    }

    const size_t length = strlen(_text);
    const size_t kept   = (length < _capacity) ? length : (_capacity - 1);

    memcpy(_field,
           _text,
           kept);
    _field[kept] = '\0';

    return;
}

/*
d_internal_ssl_openssl_version
  The negotiated version as the kernel names it: the library's version is the
same wire code point. NONE for any the kernel does not name.
*/
static enum d_ssl_version
d_internal_ssl_openssl_version(
    const SSL* _library
)
{
    const int version = SSL_version(_library);

    switch (version)
    {
        case D_SSL_VERSION_TLS1_0:
            return D_SSL_VERSION_TLS1_0;

        case D_SSL_VERSION_TLS1_1:
            return D_SSL_VERSION_TLS1_1;

        case D_SSL_VERSION_TLS1_2:
            return D_SSL_VERSION_TLS1_2;

        case D_SSL_VERSION_TLS1_3:
            return D_SSL_VERSION_TLS1_3;

        default:
            return D_SSL_VERSION_NONE;
    }
}

#if (D_ENV_SSL_OPENSSL_HAS_CIPHER_IANA == 1)

/*
d_internal_ssl_openssl_describe_cipher
  Reports the agreed suite by its IANA code point and its IANA name. A
library built without its table of standard names answers NULL for the
name, and its own name stands in.
*/
static void
d_internal_ssl_openssl_describe_cipher(
    const SSL_CIPHER*  _cipher,
    struct d_ssl_info* _out
)
{
    const char* const standard = SSL_CIPHER_standard_name(_cipher);
    const char* const name     = (standard != NULL)
                                     ? standard
                                     : SSL_CIPHER_get_name(_cipher);

    _out->cipher_id = SSL_CIPHER_get_protocol_id(_cipher);
    d_internal_ssl_openssl_copy_text(_out->cipher,
                                     sizeof(_out->cipher),
                                     name);

    return;
}

#else

/*
d_internal_ssl_openssl_describe_cipher
  Reports the agreed suite by the library's own name. Without the calls that
give its IANA code point and name, cipher_id stays 0, as for unknown.
*/
static void
d_internal_ssl_openssl_describe_cipher(
    const SSL_CIPHER*  _cipher,
    struct d_ssl_info* _out
)
{
    d_internal_ssl_openssl_copy_text(_out->cipher,
                                     sizeof(_out->cipher),
                                     SSL_CIPHER_get_name(_cipher));

    return;
}

#endif  // D_ENV_SSL_OPENSSL_HAS_CIPHER_IANA

/*
d_internal_ssl_openssl_describe_suite
  Reports what a completed handshake negotiated: the version, the suite, the
ALPN protocol, and whether an earlier session was resumed.
*/
static void
d_internal_ssl_openssl_describe_suite(
    SSL*               _library,
    struct d_ssl_info* _out
)
{
    const SSL_CIPHER* const cipher      = SSL_get_current_cipher(_library);
    const unsigned char*    alpn        = NULL;
    unsigned int            alpn_length = 0;

    _out->version = d_internal_ssl_openssl_version(_library);
    _out->resumed = (SSL_session_reused(_library) == 1);

    // the suite, where one was agreed
    if (cipher != NULL)
    {
        d_internal_ssl_openssl_describe_cipher(cipher,
                                               _out);
    }

    SSL_get0_alpn_selected(_library,
                           &alpn,
                           &alpn_length);

    // the protocol, where one was agreed and fits
    if ( (alpn != NULL)     &&
         (alpn_length > 0)  &&
         (alpn_length <= D_SSL_ALPN_PROTOCOL_MAX) )
    {
        memcpy(_out->alpn,
               alpn,
               alpn_length);
        _out->alpn[alpn_length] = '\0';
        _out->alpn_length       = alpn_length;
    }

    return;
}

/*
d_internal_ssl_openssl_verify_flags
  Maps the library's chain verdict to d_ssl_verify_flag bits: NONE for a
chain that validated, the flag naming each reason the kernel distinguishes,
and OTHER for the rest.
*/
static uint32_t
d_internal_ssl_openssl_verify_flags(
    long _verdict
)
{
    switch (_verdict)
    {
        case X509_V_OK:
            return D_SSL_VERIFY_FLAG_NONE;

        // no path to a trusted anchor
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        case X509_V_ERR_CERT_CHAIN_TOO_LONG:
        case X509_V_ERR_INVALID_CA:
        case X509_V_ERR_CERT_UNTRUSTED:
        case X509_V_ERR_CERT_REJECTED:
            return D_SSL_VERIFY_FLAG_UNTRUSTED;

        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            return D_SSL_VERIFY_FLAG_SELF_SIGNED;

        case X509_V_ERR_CERT_HAS_EXPIRED:
            return D_SSL_VERIFY_FLAG_EXPIRED;

        case X509_V_ERR_CERT_NOT_YET_VALID:
            return D_SSL_VERIFY_FLAG_NOT_YET_VALID;

        case X509_V_ERR_CERT_REVOKED:
            return D_SSL_VERIFY_FLAG_REVOKED;

        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
            return D_SSL_VERIFY_FLAG_BAD_SIGNATURE;

        case X509_V_ERR_INVALID_PURPOSE:
            return D_SSL_VERIFY_FLAG_WRONG_PURPOSE;

        case X509_V_ERR_HOSTNAME_MISMATCH:
        case X509_V_ERR_IP_ADDRESS_MISMATCH:
            return D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH;

        default:
            return D_SSL_VERIFY_FLAG_OTHER;
    }
}

/*
d_internal_ssl_openssl_describe_chain
  Reports the chain verdict where the plan checks chains; otherwise the
kernel's NOT_PERFORMED stands. A failed check keeps its reason even though
the library keeps no certificate once verification fails. X509_V_OK is the
library's default as well as a pass, so it counts only once a certificate
has arrived.
*/
static void
d_internal_ssl_openssl_describe_chain(
    const struct d_internal_ssl_openssl_session* _session,
    struct d_ssl_info*                           _out
)
{
    // a chain the plan does not check has no verdict to report
    if (!_session->context->plan.verify_chain)
    {
        return;
    }

    const long verdict = SSL_get_verify_result(_session->library);

    // a failed check, with or without the certificate it failed
    if (verdict != X509_V_OK)
    {
        _out->verify = d_internal_ssl_openssl_verify_flags(verdict);

        return;
    }

    X509* const peer = d_internal_ssl_openssl_peer(_session->library);

    // a pass needs a certificate to have passed
    if (peer != NULL)
    {
        X509_free(peer);
        _out->verify = D_SSL_VERIFY_FLAG_NONE;
    }

    return;
}

/*
d_internal_ssl_openssl_describe
  Fills the engine's part of a d_ssl_info: the alerts at any time, what was
negotiated once the handshake is complete, and the chain verdict once one
exists.
*/
static void
d_internal_ssl_openssl_describe(
    void*              _session,
    struct d_ssl_info* _out
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_out == NULL) )
    {
        return;
    }

    const struct d_internal_ssl_openssl_session* const session = _session;

    _out->alert_sent     = session->alert_sent;
    _out->alert_received = session->alert_received;

    // what was negotiated exists once the handshake is complete
    if (SSL_in_init(session->library) == 0)
    {
        d_internal_ssl_openssl_describe_suite(session->library,
                                              _out);
    }

    d_internal_ssl_openssl_describe_chain(session,
                                          _out);

    return;
}

/*
d_internal_ssl_openssl_load_names
  Replaces the session's cached subjectAltName entries with the peer
certificate's current ones: none, when there is no certificate or it carries
no such extension.
*/
static void
d_internal_ssl_openssl_load_names(
    struct d_internal_ssl_openssl_session* _session
)
{
    GENERAL_NAMES_free(_session->names);
    _session->names = NULL;

    X509* const peer = d_internal_ssl_openssl_peer(_session->library);

    // no certificate, no names
    if (peer == NULL)
    {
        return;
    }

    _session->names = X509_get_ext_d2i(peer,
                                       NID_subject_alt_name,
                                       NULL,
                                       NULL);
    X509_free(peer);

    return;
}

/*
d_internal_ssl_openssl_nth_name
  Finds the _index-th DNS or IP entry of a subjectAltName list, skipping
entries of other kinds, and points _out into the list.
*/
static bool
d_internal_ssl_openssl_nth_name(
    const GENERAL_NAMES*    _names,
    size_t                  _index,
    struct d_ssl_peer_name* _out
)
{
    const int count = sk_GENERAL_NAME_num(_names);
    size_t    seen  = 0;

    // entries in order, counting only the kinds the kernel understands
    for (int i = 0; i < count; i++)
    {
        const GENERAL_NAME* const entry = sk_GENERAL_NAME_value(_names,
                                                                i);
        const bool                dns   = (entry->type == GEN_DNS);

        // neither a DNS name nor an address, or not the one wanted
        if ( ( (!dns) &&
               (entry->type != GEN_IPADD) ) ||
             (seen++ != _index) )
        {
            continue;
        }

        const ASN1_STRING* const value  = (dns) ? entry->d.dNSName
                                                : entry->d.iPAddress;
        const int                length = ASN1_STRING_length(value);

        _out->kind       = (dns) ? D_SSL_NAME_DNS : D_SSL_NAME_IP;
        _out->value.data = ASN1_STRING_get0_data(value);
        _out->value.size = (length > 0) ? (size_t)length : 0;

        return true;
    }

    return false;
}

/*
d_internal_ssl_openssl_peer_name
  Reports the peer certificate's DNS and IP subjectAltName entries one index
at a time. Index 0 refreshes the cache the later indices read, so a walk from
0 always sees the current certificate; entries stay valid until the next
walk or the session's end, beyond the kernel's borrowing rule.
*/
static bool
d_internal_ssl_openssl_peer_name(
    void*                   _session,
    size_t                  _index,
    struct d_ssl_peer_name* _out
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_out == NULL) )
    {
        return false;
    }

    struct d_internal_ssl_openssl_session* const session = _session;

    // a walk begins at 0
    if (_index == 0)
    {
        d_internal_ssl_openssl_load_names(session);
    }

    // no certificate, or no subjectAltName extension
    if (session->names == NULL)
    {
        return false;
    }

    return d_internal_ssl_openssl_nth_name(session->names,
                                           _index,
                                           _out);
}

/*
d_internal_ssl_openssl_peer_certificate
  Reports the DER encoding of the peer's certificate, freshly encoded into
the session's cache, which holds it until the next call or the session's
end.
*/
static bool
d_internal_ssl_openssl_peer_certificate(
    void*                _session,
    struct d_pack_bytes* _out_der
)
{
    // parameter validation
    if ( (_session == NULL) ||
         (_out_der == NULL) )
    {
        return false;
    }

    struct d_internal_ssl_openssl_session* const session = _session;

    OPENSSL_free(session->der);
    session->der      = NULL;
    session->der_size = 0;

    X509* const peer = d_internal_ssl_openssl_peer(session->library);

    // the peer presented none
    if (peer == NULL)
    {
        return false;
    }

    unsigned char* der  = NULL;
    const int      size = i2d_X509(peer,
                                   &der);

    X509_free(peer);

    // the library could not encode it
    if (size <= 0)
    {
        return false;
    }

    session->der      = der;
    session->der_size = (size_t)size;
    _out_der->data    = der;
    _out_der->size    = (size_t)size;

    return true;
}

/*
d_ssl_engine_openssl
  Returns the engine table, immutable static data as the kernel requires.
Its features follow the library the build found.
*/
const struct d_ssl_engine*
d_ssl_engine_openssl(void)
{
    static const struct d_ssl_engine engine =
    {
        .name             = D_ENV_TLS_OPENSSL_VARIANT_NAME,
        .backend          = D_SSL_BACKEND_OPENSSL,
        .features         = ( D_SSL_ENGINE_ALPN          |
                              D_SSL_ENGINE_SYSTEM_TRUST  |
                              D_SSL_ENGINE_MEMORY_PEM    |
                              D_SSL_ENGINE_CLIENT_AUTH   |
                              ( (D_ENV_SSL_OPENSSL_HAS_TLS1_3 == 1)
                                    ? D_SSL_ENGINE_TLS1_3
                                    : 0 )                |
                              ( (D_ENV_SSL_OPENSSL_HAS_KEYLOG == 1)
                                    ? D_SSL_ENGINE_KEYLOG
                                    : 0 ) ),
        .context_create   = d_internal_ssl_openssl_context_create,
        .context_destroy  = d_internal_ssl_openssl_context_destroy,
        .session_create   = d_internal_ssl_openssl_session_create,
        .session_destroy  = d_internal_ssl_openssl_session_destroy,
        .feed             = d_internal_ssl_openssl_feed,
        .drain            = d_internal_ssl_openssl_drain,
        .handshake        = d_internal_ssl_openssl_handshake,
        .read             = d_internal_ssl_openssl_read,
        .write            = d_internal_ssl_openssl_write,
        .close            = d_internal_ssl_openssl_close,
        .describe         = d_internal_ssl_openssl_describe,
        .peer_name        = d_internal_ssl_openssl_peer_name,
        .peer_certificate = d_internal_ssl_openssl_peer_certificate
    };

    return &engine;
}

#else

/*
d_ssl_engine_openssl
  A build without the engine offers none.
*/
const struct d_ssl_engine*
d_ssl_engine_openssl(void)
{
    return NULL;
}

#endif  // D_INTERNAL_SSL_OPENSSL
