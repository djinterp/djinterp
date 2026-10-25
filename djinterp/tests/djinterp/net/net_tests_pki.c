/*******************************************************************************
* djinterp [net]                                                 net_tests_pki.c
*
* The certificates net's TLS suites are tested with, made for each run.
*   Every key is P-256 and every certificate is signed with SHA-256: a CA, a
* server certificate from it for localhost and 127.0.0.1, a client
* certificate from it, an expired server certificate from it, and a server
* certificate from a second CA that the tests never trust. Each valid one is
* good from a day ago to a week ahead, so nothing in a suite expires, and
* nothing private is kept in the tree. The files go to a fresh directory
* under TMPDIR, or /tmp, and leave with it. The SSL, STARTTLS and SMTP
* suites share it; it began as STARTTLS's own.
*
*
* path:      /tests/djinterp/net/net_tests_pki.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./net_tests_pki.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // FILE, fclose, fopen, remove, snprintf
#include <stdlib.h>   // getenv, mkdtemp
#include <string.h>   // memset
// posix
#include <unistd.h>  // rmdir


#if (D_ENV_TLS_HAS_OPENSSL == 1)

// openssl
#include <openssl/asn1.h>     // ASN1_INTEGER_set, ASN1_TIME
#include <openssl/ec.h>       // EVP_PKEY_CTX_set_ec_paramgen_curve_nid
#include <openssl/evp.h>      // EVP_PKEY, EVP_PKEY_CTX, EVP_sha256
#include <openssl/obj_mac.h>  // NID_*
#include <openssl/pem.h>      // PEM_write_PrivateKey, PEM_write_X509
#include <openssl/x509.h>     // X509, X509_NAME, X509_sign
#include <openssl/x509v3.h>   // X509V3_CTX, X509V3_EXT_conf_nid


// d_tests_net_pki_extension
//   struct: one X.509 extension, as an NID and its value in OpenSSL's
// configuration syntax; an NID of 0 ends a list.
struct d_tests_net_pki_extension
{
    int         nid;
    const char* value;
};

// D_TESTS_NET_PKI_CA_EXTENSIONS
//   constant: what makes a certificate a CA.
static const struct d_tests_net_pki_extension
D_TESTS_NET_PKI_CA_EXTENSIONS[] =
{
    { NID_basic_constraints,      "critical,CA:TRUE" },
    { NID_key_usage,              "critical,keyCertSign,cRLSign" },
    { NID_subject_key_identifier, "hash" },
    { 0,                          NULL }
};

// D_TESTS_NET_PKI_SERVER_EXTENSIONS
//   constant: a TLS server's certificate for localhost and 127.0.0.1.
static const struct d_tests_net_pki_extension
D_TESTS_NET_PKI_SERVER_EXTENSIONS[] =
{
    { NID_basic_constraints,        "critical,CA:FALSE" },
    { NID_key_usage,                "critical,digitalSignature" },
    { NID_ext_key_usage,            "serverAuth" },
    { NID_subject_alt_name,         "DNS:localhost,IP:127.0.0.1" },
    { NID_subject_key_identifier,   "hash" },
    { NID_authority_key_identifier, "keyid:always" },
    { 0,                            NULL }
};

// D_TESTS_NET_PKI_CLIENT_EXTENSIONS
//   constant: a TLS client's certificate.
static const struct d_tests_net_pki_extension
D_TESTS_NET_PKI_CLIENT_EXTENSIONS[] =
{
    { NID_basic_constraints,        "critical,CA:FALSE" },
    { NID_key_usage,                "critical,digitalSignature" },
    { NID_ext_key_usage,            "clientAuth" },
    { NID_subject_key_identifier,   "hash" },
    { NID_authority_key_identifier, "keyid:always" },
    { 0,                            NULL }
};

// d_tests_net_pki_validity
//   struct: when a certificate is good, in days from now.
struct d_tests_net_pki_validity
{
    int from;
    int until;
};

/*
d_tests_net_pki_key
  A fresh P-256 key, or NULL.
*/
static EVP_PKEY*
d_tests_net_pki_key(
    void
)
{
    EVP_PKEY*     key     = NULL;
    EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id(EVP_PKEY_EC,
                                                NULL);

    if ( (!context)                           ||
         (EVP_PKEY_keygen_init(context) <= 0) ||
         (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(context,
                                                 NID_X9_62_prime256v1) <=
          0)                                  ||
         (EVP_PKEY_keygen(context,
                          &key) <= 0) )
    {
        EVP_PKEY_free(key);
        key = NULL;
    }

    EVP_PKEY_CTX_free(context);

    return key;
}

/*
d_tests_net_pki_extend
  Adds `_extensions` to `_certificate`, which `_issuer` signs; a CA signs
itself.
*/
static bool
d_tests_net_pki_extend(
    X509*                                    _certificate,
    X509*                                    _issuer,
    const struct d_tests_net_pki_extension* _extensions
)
{
    X509V3_CTX context;
    bool       added = true;

    memset(&context,
           0,
           sizeof(context));
    X509V3_set_ctx(&context,
                   (_issuer) ? _issuer : _certificate,
                   _certificate,
                   NULL,
                   NULL,
                   0);

    // each extension in order, until one fails
    for (size_t i = 0; (added) && (_extensions[i].nid != 0); ++i)
    {
        X509_EXTENSION* extension = X509V3_EXT_conf_nid(NULL,
                                                        &context,
                                                        _extensions[i].nid,
                                                        _extensions[i].value);

        added = ( (extension != NULL) &&
                  (X509_add_ext(_certificate,
                                extension,
                                -1) == 1) );
        X509_EXTENSION_free(extension);
    }

    return added;
}

// d_tests_net_pki_profile
//   struct: what one certificate says: its subject's common name, its serial
// number, when it is good, and its extensions.
struct d_tests_net_pki_profile
{
    const char*                              name;
    long                                     serial;
    struct d_tests_net_pki_validity         validity;
    const struct d_tests_net_pki_extension* extensions;
};

// d_tests_net_pki_signer
//   struct: who signs a certificate: its issuer's certificate and key, both
// NULL for a certificate that signs itself.
struct d_tests_net_pki_signer
{
    X509*     certificate;
    EVP_PKEY* key;
};

/*
d_tests_net_pki_certificate
  A certificate for `_key` as `_profile` describes it, signed by `_signer`,
or by `_key` itself when the signer is empty. NULL on any failure.
*/
static X509*
d_tests_net_pki_certificate(
    EVP_PKEY*                              _key,
    const struct d_tests_net_pki_profile* _profile,
    struct d_tests_net_pki_signer         _signer
)
{
    X509* certificate = X509_new();

    if (!certificate)
    {
        return NULL;
    }

    X509_NAME* subject = X509_get_subject_name(certificate);
    X509_NAME* issuer  = (_signer.certificate)
                             ? X509_get_subject_name(_signer.certificate)
                             : subject;
    bool       built   =
        ( (X509_set_version(certificate,
                            2) == 1)                &&
          (ASN1_INTEGER_set(X509_get_serialNumber(certificate),
                            _profile->serial) == 1) &&
          (X509_time_adj_ex(X509_getm_notBefore(certificate),
                            _profile->validity.from,
                            0,
                            NULL) != NULL)          &&
          (X509_time_adj_ex(X509_getm_notAfter(certificate),
                            _profile->validity.until,
                            0,
                            NULL) != NULL)          &&
          (X509_set_pubkey(certificate,
                           _key) == 1)              &&
          (X509_NAME_add_entry_by_txt(subject,
                                      "CN",
                                      MBSTRING_ASC,
                                      (const unsigned char*)_profile->name,
                                      -1,
                                      -1,
                                      0) == 1)      &&
          (X509_set_issuer_name(certificate,
                                issuer) == 1) );

    built = ( (built)                                             &&
              (d_tests_net_pki_extend(certificate,
                                      _signer.certificate,
                                      _profile->extensions)) &&
              (X509_sign(certificate,
                         (_signer.key) ? _signer.key : _key,
                         EVP_sha256()) > 0) );

    if (!built)
    {
        X509_free(certificate);

        return NULL;
    }

    return certificate;
}

/*
d_tests_net_pki_path
  `_directory`/`_file` into `_out`; false if it does not fit.
*/
static bool
d_tests_net_pki_path(
    char*       _out,
    size_t      _capacity,
    const char* _directory,
    const char* _file
)
{
    const int written = snprintf(_out,
                                 _capacity,
                                 "%s/%s",
                                 _directory,
                                 _file);

    return ( (written > 0) &&
             ((size_t)written < _capacity) );
}

/*
d_tests_net_pki_write
  Writes a certificate, or with `_certificate` NULL a private key, as PEM.
*/
static bool
d_tests_net_pki_write(
    const char* _path,
    X509*       _certificate,
    EVP_PKEY*   _key
)
{
    FILE* file = fopen(_path,
                       "w");

    if (!file)
    {
        return false;
    }

    const bool written = (_certificate)
                             ? (PEM_write_X509(file,
                                               _certificate) == 1)
                             : (PEM_write_PrivateKey(file,
                                                     _key,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     NULL,
                                                     NULL) == 1);

    return ( (fclose(file) == 0) &&
             (written) );
}

// d_tests_net_pki_parts
//   struct: the keys and certificates of one PKI, while it is being made.
struct d_tests_net_pki_parts
{
    EVP_PKEY* ca_key;
    EVP_PKEY* rogue_ca_key;
    EVP_PKEY* server_key;
    EVP_PKEY* client_key;
    EVP_PKEY* expired_key;
    EVP_PKEY* rogue_key;
    X509*     ca;
    X509*     rogue_ca;
    X509*     server;
    X509*     client;
    X509*     expired;
    X509*     rogue;
};

/*
d_tests_net_pki_keys
  Makes every key; false if any one failed.
*/
static bool
d_tests_net_pki_keys(
    struct d_tests_net_pki_parts* _parts
)
{
    _parts->ca_key       = d_tests_net_pki_key();
    _parts->rogue_ca_key = d_tests_net_pki_key();
    _parts->server_key   = d_tests_net_pki_key();
    _parts->client_key   = d_tests_net_pki_key();
    _parts->expired_key  = d_tests_net_pki_key();
    _parts->rogue_key    = d_tests_net_pki_key();

    return ( (_parts->ca_key)       &&
             (_parts->rogue_ca_key) &&
             (_parts->server_key)   &&
             (_parts->client_key)   &&
             (_parts->expired_key)  &&
             (_parts->rogue_key) );
}

/*
d_tests_net_pki_authorities
  Makes the two CAs, each signing itself; false if either failed.
*/
static bool
d_tests_net_pki_authorities(
    struct d_tests_net_pki_parts* _parts
)
{
    const struct d_tests_net_pki_signer  itself  =
    {
        .certificate = NULL,
        .key         = NULL
    };
    const struct d_tests_net_pki_profile trusted =
    {
        .name       = "djinterp test CA",
        .serial     = 1,
        .validity   = { .from = -1, .until = 7 },
        .extensions = D_TESTS_NET_PKI_CA_EXTENSIONS
    };
    const struct d_tests_net_pki_profile rogue   =
    {
        .name       = "djinterp untrusted CA",
        .serial     = 2,
        .validity   = { .from = -1, .until = 7 },
        .extensions = D_TESTS_NET_PKI_CA_EXTENSIONS
    };

    _parts->ca       = d_tests_net_pki_certificate(_parts->ca_key,
                                                   &trusted,
                                                   itself);
    _parts->rogue_ca = d_tests_net_pki_certificate(_parts->rogue_ca_key,
                                                   &rogue,
                                                   itself);

    return ( (_parts->ca) &&
             (_parts->rogue_ca) );
}

/*
d_tests_net_pki_leaves
  Makes the four end certificates: the server's, the client's, and the
expired one from the trusted CA, and the server's from the untrusted one;
false if any failed.
*/
static bool
d_tests_net_pki_leaves(
    struct d_tests_net_pki_parts* _parts
)
{
    const struct d_tests_net_pki_signer  ca       = { _parts->ca,
                                                       _parts->ca_key };
    const struct d_tests_net_pki_signer  rogue    = { _parts->rogue_ca,
                                                       _parts->rogue_ca_key };
    const struct d_tests_net_pki_profile server   =
    {
        .name       = "localhost",
        .serial     = 3,
        .validity   = { .from = -1, .until = 7 },
        .extensions = D_TESTS_NET_PKI_SERVER_EXTENSIONS
    };
    const struct d_tests_net_pki_profile client   =
    {
        .name       = "client.example",
        .serial     = 4,
        .validity   = { .from = -1, .until = 7 },
        .extensions = D_TESTS_NET_PKI_CLIENT_EXTENSIONS
    };
    struct d_tests_net_pki_profile       expired  = server;
    struct d_tests_net_pki_profile       impostor = server;

    expired.serial   = 5;
    expired.validity = (struct d_tests_net_pki_validity){ .from  = -10,
                                                           .until = -3 };
    impostor.serial  = 6;
    _parts->server   = d_tests_net_pki_certificate(_parts->server_key,
                                                   &server,
                                                   ca);
    _parts->client   = d_tests_net_pki_certificate(_parts->client_key,
                                                   &client,
                                                   ca);
    _parts->expired  = d_tests_net_pki_certificate(_parts->expired_key,
                                                   &expired,
                                                   ca);
    _parts->rogue    = d_tests_net_pki_certificate(_parts->rogue_key,
                                                   &impostor,
                                                   rogue);

    return ( (_parts->server)  &&
             (_parts->client)  &&
             (_parts->expired) &&
             (_parts->rogue) );
}

/*
d_tests_net_pki_build
  Makes every key and certificate; false if any one failed. What was made is
in `_parts` either way, for d_tests_net_pki_free.
*/
static bool
d_tests_net_pki_build(
    struct d_tests_net_pki_parts* _parts
)
{
    return ( (d_tests_net_pki_keys(_parts))        &&
             (d_tests_net_pki_authorities(_parts)) &&
             (d_tests_net_pki_leaves(_parts)) );
}

/*
d_tests_net_pki_free
  Frees whatever d_tests_net_pki_build made; each free accepts NULL.
*/
static void
d_tests_net_pki_free(
    struct d_tests_net_pki_parts* _parts
)
{
    X509_free(_parts->ca);
    X509_free(_parts->rogue_ca);
    X509_free(_parts->server);
    X509_free(_parts->client);
    X509_free(_parts->expired);
    X509_free(_parts->rogue);
    EVP_PKEY_free(_parts->ca_key);
    EVP_PKEY_free(_parts->rogue_ca_key);
    EVP_PKEY_free(_parts->server_key);
    EVP_PKEY_free(_parts->client_key);
    EVP_PKEY_free(_parts->expired_key);
    EVP_PKEY_free(_parts->rogue_key);

    return;
}

/*
d_tests_net_pki_paths
  Names every file in `_pki->directory`.
*/
static bool
d_tests_net_pki_paths(
    struct d_tests_net_pki* _pki
)
{
    const char* const directory = _pki->directory;

    return ( (d_tests_net_pki_path(_pki->ca,
                                   sizeof(_pki->ca),
                                   directory,
                                   "ca.pem"))      &&
             (d_tests_net_pki_path(_pki->server_cert,
                                   sizeof(_pki->server_cert),
                                   directory,
                                   "server.pem"))  &&
             (d_tests_net_pki_path(_pki->server_key,
                                   sizeof(_pki->server_key),
                                   directory,
                                   "server.key"))  &&
             (d_tests_net_pki_path(_pki->client_cert,
                                   sizeof(_pki->client_cert),
                                   directory,
                                   "client.pem"))  &&
             (d_tests_net_pki_path(_pki->client_key,
                                   sizeof(_pki->client_key),
                                   directory,
                                   "client.key"))  &&
             (d_tests_net_pki_path(_pki->expired_cert,
                                   sizeof(_pki->expired_cert),
                                   directory,
                                   "expired.pem")) &&
             (d_tests_net_pki_path(_pki->expired_key,
                                   sizeof(_pki->expired_key),
                                   directory,
                                   "expired.key")) &&
             (d_tests_net_pki_path(_pki->rogue_cert,
                                   sizeof(_pki->rogue_cert),
                                   directory,
                                   "rogue.pem"))   &&
             (d_tests_net_pki_path(_pki->rogue_key,
                                   sizeof(_pki->rogue_key),
                                   directory,
                                   "rogue.key")) );
}

/*
d_tests_net_pki_make
  The directory first, then the keys and certificates, then the files; a
failure part-way removes whatever was written.
*/
bool
d_tests_net_pki_make(
    struct d_tests_net_pki* _pki
)
{
    struct d_tests_net_pki_parts parts =
    {
        .ca_key = NULL
    };
    const char* base = getenv("TMPDIR");

    memset(_pki,
           0,
           sizeof(*_pki));

    if ( (!base) ||
         (base[0] == '\0') )
    {
        base = "/tmp";
    }

    if ( (!d_tests_net_pki_path(_pki->directory,
                                sizeof(_pki->directory),
                                base,
                                "d_net_pki_XXXXXX")) ||
         (!mkdtemp(_pki->directory)) )
    {
        _pki->directory[0] = '\0';

        return false;
    }

    const bool made =
        ( (d_tests_net_pki_paths(_pki))                   &&
          (d_tests_net_pki_build(&parts))                 &&
          (d_tests_net_pki_write(_pki->ca,
                                 parts.ca,
                                 NULL))              &&
          (d_tests_net_pki_write(_pki->server_cert,
                                 parts.server,
                                 NULL))              &&
          (d_tests_net_pki_write(_pki->server_key,
                                 NULL,
                                 parts.server_key))  &&
          (d_tests_net_pki_write(_pki->client_cert,
                                 parts.client,
                                 NULL))              &&
          (d_tests_net_pki_write(_pki->client_key,
                                 NULL,
                                 parts.client_key))  &&
          (d_tests_net_pki_write(_pki->expired_cert,
                                 parts.expired,
                                 NULL))              &&
          (d_tests_net_pki_write(_pki->expired_key,
                                 NULL,
                                 parts.expired_key)) &&
          (d_tests_net_pki_write(_pki->rogue_cert,
                                 parts.rogue,
                                 NULL))              &&
          (d_tests_net_pki_write(_pki->rogue_key,
                                 NULL,
                                 parts.rogue_key)) );

    d_tests_net_pki_free(&parts);

    if (!made)
    {
        d_tests_net_pki_remove(_pki);
    }

    return made;
}

#else  // D_ENV_TLS_HAS_OPENSSL

/*
d_tests_net_pki_make
  Without an OpenSSL-family library there is nothing to make certificates
with, and nothing that would use them.
*/
bool
d_tests_net_pki_make(
    struct d_tests_net_pki* _pki
)
{
    memset(_pki,
           0,
           sizeof(*_pki));

    return false;
}

#endif  // D_ENV_TLS_HAS_OPENSSL

/*
d_tests_net_pki_remove
  Removes each file that was named, then the directory. Files that were
never written are simply not there to remove.
*/
void
d_tests_net_pki_remove(
    const struct d_tests_net_pki* _pki
)
{
    const char* const files[] =
    {
        _pki->ca,
        _pki->server_cert,
        _pki->server_key,
        _pki->client_cert,
        _pki->client_key,
        _pki->expired_cert,
        _pki->expired_key,
        _pki->rogue_cert,
        _pki->rogue_key
    };

    if (_pki->directory[0] == '\0')
    {
        return;
    }

    // every file, then the directory that held them
    for (size_t i = 0; i < (sizeof(files) / sizeof(files[0])); ++i)
    {
        if (files[i][0] != '\0')
        {
            (void)remove(files[i]);
        }
    }

    (void)rmdir(_pki->directory);

    return;
}
