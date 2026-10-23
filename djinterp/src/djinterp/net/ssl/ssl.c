/*******************************************************************************
* djinterp [net]                                                           ssl.c
*
* Implementation of the shared SSL/TLS kernel declared in ssl.h.
*   Pure computation over caller-owned memory -- names, ALPN lists, PEM,
* fingerprints, configuration -- and the session pump, which moves ciphertext
* between an engine and the net.h connection a session runs over. The only
* I/O is through the connections the caller passes in, and the only TLS
* through the engine it names. A session is a connection itself, its
* operations the session calls in net.h's terms.
*
*
* path:      /src/djinterp/net/ssl/ssl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssl/ssl.h"  // corresponding header
// std
#include <string.h>  // memchr, memcmp, memcpy, memmove, memset, strlen
// djinterp
#include "../../../../inc/djinterp/env/net/ssl/env_ssl.h"     // D_ENV_SSL_CA_FILES
#include "../../../../inc/djinterp/net/codec/codec_base64.h"  // d_net_base64_decode
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // d_net_digest_*
#include "../../../../inc/djinterp/net/codec/codec_hex.h"     // d_net_hex_*
#include "../../../../inc/djinterp/net/net_common.h"          // connections


//==============================================================================
// 1.  TEXT AND BYTE HELPERS
//==============================================================================

/*
d_internal_ssl_text
  Builds a text span. Exists so every span in this file is spelled one way, and
so a zero-length span never carries arithmetic on a NULL base.
*/
static struct d_pack_text
d_internal_ssl_text(
    const char* _data,
    size_t      _length
)
{
    struct d_pack_text text = { _data, _length };

    return text;
}

/*
d_internal_ssl_bytes
  Builds a byte span, for the same reasons as d_internal_ssl_text.
*/
static struct d_pack_bytes
d_internal_ssl_bytes(
    const void* _data,
    size_t      _size
)
{
    struct d_pack_bytes bytes = { _data, _size };

    return bytes;
}

/*
d_internal_ssl_text_ok
  A span is usable when it has storage or is empty. A NULL base with a non-zero
length is a caller bug, and every public entry point rejects it here rather
than dereferencing it later.
*/
static bool
d_internal_ssl_text_ok(
    struct d_pack_text _text
)
{
    return ( (_text.data != NULL) ||
             (_text.length == 0) );
}

/*
d_internal_ssl_bytes_ok
  The byte-span counterpart of d_internal_ssl_text_ok.
*/
static bool
d_internal_ssl_bytes_ok(
    struct d_pack_bytes _bytes
)
{
    return ( (_bytes.data != NULL) ||
             (_bytes.size == 0) );
}

/*
d_internal_ssl_strip_dot
  Drops one trailing dot: "example.com." and "example.com" name the same host.
Only one, so "example.com.." stays malformed.
*/
static struct d_pack_text
d_internal_ssl_strip_dot(
    struct d_pack_text _host
)
{
    if ( (_host.length > 0) &&
         (_host.data[_host.length - 1] == '.') )
    {
        return d_internal_ssl_text(_host.data,
                                   _host.length - 1);
    }

    return _host;
}

/*
d_internal_ssl_find
  The index of the first occurrence of the NUL-terminated _needle in _text at
or after _from, or _text.length when there is none.
*/
static size_t
d_internal_ssl_find(
    struct d_pack_text _text,
    size_t             _from,
    const char*        _needle
)
{
    size_t length = strlen(_needle);

    if (length > _text.length)
    {
        return _text.length;
    }

    for (size_t i = _from; i + length <= _text.length; ++i)
    {
        if (memcmp(_text.data + i,
                   _needle,
                   length) == 0)
        {
            return i;
        }
    }

    return _text.length;
}

//==============================================================================
// 2.  OUTPUT HELPERS
//==============================================================================

/*
d_internal_ssl_out_args_ok
  The two-call protocol's argument check, shared by every producer: a size
pointer is mandatory, and a NULL buffer is only legal with zero capacity.
*/
static bool
d_internal_ssl_out_args_ok(
    const void*   _out,
    size_t        _capacity,
    const size_t* _out_size
)
{
    return ( (_out_size != NULL) &&
             ( (_out != NULL) ||
               (_capacity == 0) ) );
}

/*
d_internal_ssl_out_check
  Applies the tail of the two-call protocol once the exact size is known:
record it, then decide between measure, too-small, and produce. Every producer
reaches its byte-writing step only through here, so none can write past a
buffer. _reserve counts bytes needed beyond _size, such as a NUL.
*/
static enum d_ssl_error
d_internal_ssl_out_check(
    const void* _out,
    size_t      _capacity,
    size_t      _size,
    size_t      _reserve,
    size_t*     _out_size
)
{
    *_out_size = _size;

    // a measure pass stops here
    if ( (_out == NULL) &&
         (_capacity == 0) )
    {
        return D_SSL_OK;
    }

    // the requirement is already recorded, so a caller can grow and retry
    if ( (_capacity < _reserve) ||
         (_capacity - _reserve < _size) )
    {
        return D_SSL_ERROR_BUFFER_TOO_SMALL;
    }

    return D_SSL_OK;
}

/*
d_internal_ssl_put
  Appends _length bytes at *_pos. Callers have already sized the destination
through d_internal_ssl_out_check.
*/
static void
d_internal_ssl_put(
    char*       _out,
    size_t*     _pos,
    const char* _data,
    size_t      _length
)
{
    if (_length > 0)
    {
        memcpy(_out + *_pos,
               _data,
               _length);
        *_pos += _length;
    }

    return;
}

//==============================================================================
// 3.  STATUS
//==============================================================================

/*
d_ssl_error_name
  A switch rather than a table: the values are sparse across two ranges.
*/
const char*
d_ssl_error_name(
    enum d_ssl_error _error
)
{
    switch (_error)
    {
        case D_SSL_OK:                     return "OK";
        case D_SSL_ERROR_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case D_SSL_ERROR_MALFORMED:        return "MALFORMED";
        case D_SSL_ERROR_NOT_FOUND:        return "NOT_FOUND";
        case D_SSL_ERROR_UNSUPPORTED:      return "UNSUPPORTED";
        case D_SSL_ERROR_CONFIG:           return "CONFIG";
        case D_SSL_ERROR_STATE:            return "STATE";
        case D_SSL_ERROR_NOT_TLS:          return "NOT_TLS";
        case D_SSL_ERROR_HANDSHAKE:        return "HANDSHAKE";
        case D_SSL_ERROR_VERIFY:           return "VERIFY";
        case D_SSL_ERROR_PROTOCOL:         return "PROTOCOL";
        case D_SSL_ERROR_TRUNCATED:        return "TRUNCATED";
        case D_SSL_ERROR_BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
        case D_SSL_ERROR_WOULD_BLOCK:      return "WOULD_BLOCK";
        case D_SSL_ERROR_TRANSPORT:        return "TRANSPORT";
        case D_SSL_ERROR_CLOSED:           return "CLOSED";
        case D_SSL_ERROR_WANT_READ:        return "WANT_READ";
        case D_SSL_ERROR_OUT_OF_MEMORY:    return "OUT_OF_MEMORY";
        case D_SSL_ERROR_BACKEND:          return "BACKEND";
    }

    return NULL;
}

/*
d_ssl_error_is_formal
  Everything above OK and below the mechanical floor.
*/
bool
d_ssl_error_is_formal(
    enum d_ssl_error _error
)
{
    return ( ((int)_error > (int)D_SSL_OK) &&
             ((int)_error < D_SSL_ERROR_MECHANICAL_FLOOR) );
}

/*
d_ssl_error_is_mechanical
  Everything at or above the mechanical floor.
*/
bool
d_ssl_error_is_mechanical(
    enum d_ssl_error _error
)
{
    return ((int)_error >= D_SSL_ERROR_MECHANICAL_FLOOR);
}

/*
d_ssl_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_ssl_error_to_net(
    enum d_ssl_error _error
)
{
    switch (_error)
    {
        case D_SSL_OK:
            return D_NET_OK;
        case D_SSL_ERROR_INVALID_ARGUMENT:
        case D_SSL_ERROR_MALFORMED:
        case D_SSL_ERROR_CONFIG:
        case D_SSL_ERROR_STATE:
        case D_SSL_ERROR_BUFFER_TOO_SMALL:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_SSL_ERROR_NOT_TLS:
        case D_SSL_ERROR_HANDSHAKE:
        case D_SSL_ERROR_PROTOCOL:
            return D_NET_ERROR_PROTOCOL;
        case D_SSL_ERROR_TRUNCATED:
        case D_SSL_ERROR_CLOSED:
            return D_NET_ERROR_CLOSED;
        case D_SSL_ERROR_WOULD_BLOCK:
        case D_SSL_ERROR_WANT_READ:
            return D_NET_ERROR_WOULD_BLOCK;
        case D_SSL_ERROR_OUT_OF_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        case D_SSL_ERROR_VERIFY:
            return D_NET_ERROR_ACCESS_DENIED;
        case D_SSL_ERROR_NOT_FOUND:
        case D_SSL_ERROR_UNSUPPORTED:
        case D_SSL_ERROR_TRANSPORT:
        case D_SSL_ERROR_BACKEND:
            return D_NET_ERROR_UNKNOWN;
    }

    return D_NET_ERROR_UNKNOWN;
}

//==============================================================================
// 4.  VOCABULARY
//==============================================================================

// d_ssl_backend mirrors env_tls.h's identifiers, so a run-time backend can be
// compared with D_ENV_SSL_BACKEND
D_STATIC_ASSERT((int)D_SSL_BACKEND_NONE == D_ENV_TLS_BACKEND_NONE,
                "d_ssl_backend: NONE drifted from env_tls.h");
D_STATIC_ASSERT((int)D_SSL_BACKEND_OPENSSL == D_ENV_TLS_BACKEND_OPENSSL,
                "d_ssl_backend: OPENSSL drifted from env_tls.h");
D_STATIC_ASSERT((int)D_SSL_BACKEND_MBEDTLS == D_ENV_TLS_BACKEND_MBEDTLS,
                "d_ssl_backend: MBEDTLS drifted from env_tls.h");
D_STATIC_ASSERT((int)D_SSL_BACKEND_GNUTLS == D_ENV_TLS_BACKEND_GNUTLS,
                "d_ssl_backend: GNUTLS drifted from env_tls.h");
D_STATIC_ASSERT((int)D_SSL_BACKEND_WOLFSSL == D_ENV_TLS_BACKEND_WOLFSSL,
                "d_ssl_backend: WOLFSSL drifted from env_tls.h");
D_STATIC_ASSERT((int)D_SSL_BACKEND_SCHANNEL == D_ENV_TLS_BACKEND_SCHANNEL,
                "d_ssl_backend: SCHANNEL drifted from env_tls.h");
D_STATIC_ASSERT( (int)D_SSL_BACKEND_SECURETRANSPORT ==
                 D_ENV_TLS_BACKEND_SECURETRANSPORT,
                "d_ssl_backend: SECURETRANSPORT drifted from env_tls.h");

// d_internal_ssl_alert_entry
//   struct: one alert's RFC name and what it means for a session.
struct d_internal_ssl_alert_entry
{
    int32_t          alert;
    const char*      name;
    enum d_ssl_error status;
};

// d_internal_ssl_alerts
//   table: every assigned alert, retired ones included, in value order.
static const struct d_internal_ssl_alert_entry d_internal_ssl_alerts[] =
{
    { D_SSL_ALERT_CLOSE_NOTIFY,             "close_notify",
      D_SSL_ERROR_CLOSED },
    { D_SSL_ALERT_UNEXPECTED_MESSAGE,       "unexpected_message",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_BAD_RECORD_MAC,           "bad_record_mac",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_DECRYPTION_FAILED,        "decryption_failed",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_RECORD_OVERFLOW,          "record_overflow",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_DECOMPRESSION_FAILURE,    "decompression_failure",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_HANDSHAKE_FAILURE,        "handshake_failure",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_NO_CERTIFICATE,           "no_certificate",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_BAD_CERTIFICATE,          "bad_certificate",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_UNSUPPORTED_CERTIFICATE,  "unsupported_certificate",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_CERTIFICATE_REVOKED,      "certificate_revoked",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_CERTIFICATE_EXPIRED,      "certificate_expired",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_CERTIFICATE_UNKNOWN,      "certificate_unknown",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_ILLEGAL_PARAMETER,        "illegal_parameter",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_UNKNOWN_CA,               "unknown_ca",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_ACCESS_DENIED,            "access_denied",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_DECODE_ERROR,             "decode_error",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_DECRYPT_ERROR,            "decrypt_error",
      D_SSL_ERROR_PROTOCOL },
    { D_SSL_ALERT_EXPORT_RESTRICTION,       "export_restriction",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_PROTOCOL_VERSION,         "protocol_version",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_INSUFFICIENT_SECURITY,    "insufficient_security",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_INTERNAL_ERROR,           "internal_error",
      D_SSL_ERROR_BACKEND },
    { D_SSL_ALERT_INAPPROPRIATE_FALLBACK,   "inappropriate_fallback",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_USER_CANCELED,            "user_canceled",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_NO_RENEGOTIATION,         "no_renegotiation",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_MISSING_EXTENSION,        "missing_extension",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_UNSUPPORTED_EXTENSION,    "unsupported_extension",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_CERTIFICATE_UNOBTAINABLE, "certificate_unobtainable",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_UNRECOGNIZED_NAME,        "unrecognized_name",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE,
      "bad_certificate_status_response",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_BAD_CERTIFICATE_HASH_VALUE,
      "bad_certificate_hash_value",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_UNKNOWN_PSK_IDENTITY,     "unknown_psk_identity",
      D_SSL_ERROR_HANDSHAKE },
    { D_SSL_ALERT_CERTIFICATE_REQUIRED,     "certificate_required",
      D_SSL_ERROR_VERIFY },
    { D_SSL_ALERT_NO_APPLICATION_PROTOCOL,  "no_application_protocol",
      D_SSL_ERROR_HANDSHAKE }
};

/*
d_internal_ssl_alert_find
  The table entry for an alert value, or NULL for an unassigned one.
*/
static const struct d_internal_ssl_alert_entry*
d_internal_ssl_alert_find(
    int32_t _alert
)
{
    size_t count = sizeof(d_internal_ssl_alerts) /
                   sizeof(d_internal_ssl_alerts[0]);

    for (size_t i = 0; i < count; ++i)
    {
        if (d_internal_ssl_alerts[i].alert == _alert)
        {
            return &d_internal_ssl_alerts[i];
        }
    }

    return NULL;
}

/*
d_ssl_version_name
  The spellings OpenSSL-derived logs and configuration files use, with the
minor version always written out.
*/
const char*
d_ssl_version_name(
    enum d_ssl_version _version
)
{
    switch (_version)
    {
        case D_SSL_VERSION_SSL3:   return "SSLv3";
        case D_SSL_VERSION_TLS1_0: return "TLSv1.0";
        case D_SSL_VERSION_TLS1_1: return "TLSv1.1";
        case D_SSL_VERSION_TLS1_2: return "TLSv1.2";
        case D_SSL_VERSION_TLS1_3: return "TLSv1.3";
        case D_SSL_VERSION_NONE:   break;
    }

    return NULL;
}

/*
d_ssl_version_is_known
  SSL 3.0 through TLS 1.3: every version the kernel can name.
*/
bool
d_ssl_version_is_known(
    enum d_ssl_version _version
)
{
    return ( ((int)_version >= (int)D_SSL_VERSION_SSL3) &&
             ((int)_version <= (int)D_SSL_VERSION_TLS1_3) );
}

/*
d_ssl_version_is_deprecated
  RFC 7568 retired SSL 3.0 and RFC 8996 TLS 1.0 and 1.1.
*/
bool
d_ssl_version_is_deprecated(
    enum d_ssl_version _version
)
{
    return ( (d_ssl_version_is_known(_version)) &&
             ((int)_version < (int)D_SSL_VERSION_TLS1_2) );
}

/*
d_internal_ssl_version_digits
  The digits that end a version name, from `_pos`: the major digit, then,
optionally, a dot or an underscore and the minor digit, and nothing after.
*/
static bool
d_internal_ssl_version_digits(
    struct d_pack_text _name,
    size_t             _pos,
    int*               _major,
    int*               _minor
)
{
    size_t pos = _pos;

    // the major digit
    if ( (pos >= _name.length) ||
         (!d_net_ascii_is_digit(_name.data[pos])) )
    {
        return false;
    }

    *_major = _name.data[pos++] - '0';

    // an optional minor, the last character
    if (pos < _name.length)
    {
        if ( ( (_name.data[pos] != '.') &&
               (_name.data[pos] != '_') ) ||
             (pos + 2 != _name.length) ||
             (!d_net_ascii_is_digit(_name.data[pos + 1])) )
        {
            return false;
        }

        *_minor = _name.data[pos + 1] - '0';
    }

    return true;
}

/*
d_ssl_version_from_name
  A family prefix ("tls" or "ssl", optional for TLS), at most one separator
('v', space, or underscore), then the version: "1" through "1.3" for TLS, "3"
or "3.0" for SSL, with a dot or an underscore before the minor digit.
*/
enum d_ssl_version
d_ssl_version_from_name(
    struct d_pack_text _name
)
{
    struct d_pack_text name  = { NULL, 0 };
    bool               ssl   = false;
    size_t             pos   = 0;
    int                major = 0;
    int                minor = 0;

    // parameter validation
    if (!d_internal_ssl_text_ok(_name))
    {
        return D_SSL_VERSION_NONE;
    }

    name = d_net_ascii_trim_space(_name);

    // the family prefix
    if (d_net_ascii_istarts(name,
                            "tls"))
    {
        pos = 3;
    }
    else if (d_net_ascii_istarts(name,
                                 "ssl"))
    {
        ssl = true;
        pos = 3;
    }

    // one separator may follow a prefix
    if ( (pos > 0) &&
         (pos < name.length) &&
         ( (d_net_ascii_lower(name.data[pos]) == 'v') ||
           (name.data[pos] == ' ')                    ||
           (name.data[pos] == '_') ) )
    {
        ++pos;
    }

    // the major digit, then an optional minor
    if (!d_internal_ssl_version_digits(name,
                                       pos,
                                       &major,
                                       &minor))
    {
        return D_SSL_VERSION_NONE;
    }

    if (ssl)
    {
        return ( (major == 3) &&
                 (minor == 0) ) ? D_SSL_VERSION_SSL3 : D_SSL_VERSION_NONE;
    }

    if ( (major != 1) ||
         (minor > 3) )
    {
        return D_SSL_VERSION_NONE;
    }

    return (enum d_ssl_version)((int)D_SSL_VERSION_TLS1_0 + minor);
}

/*
d_ssl_role_name
  "client" or "server".
*/
const char*
d_ssl_role_name(
    enum d_ssl_role _role
)
{
    switch (_role)
    {
        case D_SSL_ROLE_CLIENT: return "client";
        case D_SSL_ROLE_SERVER: return "server";
    }

    return NULL;
}

/*
d_ssl_backend_name
  The names env_tls.h's D_ENV_TLS_BACKEND_NAME uses, so both read alike.
*/
const char*
d_ssl_backend_name(
    enum d_ssl_backend _backend
)
{
    switch (_backend)
    {
        case D_SSL_BACKEND_NONE:            return "none";
        case D_SSL_BACKEND_OPENSSL:         return "OpenSSL";
        case D_SSL_BACKEND_MBEDTLS:         return "mbedTLS";
        case D_SSL_BACKEND_GNUTLS:          return "GnuTLS";
        case D_SSL_BACKEND_WOLFSSL:         return "wolfSSL";
        case D_SSL_BACKEND_SCHANNEL:        return "Schannel";
        case D_SSL_BACKEND_SECURETRANSPORT: return "Secure Transport";
        case D_SSL_BACKEND_OTHER:           return "other";
    }

    return NULL;
}

/*
d_ssl_alert_name
  A table lookup.
*/
const char*
d_ssl_alert_name(
    int32_t _alert
)
{
    const struct d_internal_ssl_alert_entry* entry =
        d_internal_ssl_alert_find(_alert);

    return (entry != NULL) ? entry->name : NULL;
}

/*
d_ssl_alert_error
  A table lookup; an unassigned alert is a protocol error, since a conforming
peer does not send one.
*/
enum d_ssl_error
d_ssl_alert_error(
    int32_t _alert
)
{
    const struct d_internal_ssl_alert_entry* entry = NULL;

    if (_alert == D_SSL_ALERT_NONE)
    {
        return D_SSL_OK;
    }

    entry = d_internal_ssl_alert_find(_alert);

    return (entry != NULL) ? entry->status : D_SSL_ERROR_PROTOCOL;
}

//==============================================================================
// 5.  VERIFICATION
//==============================================================================

// d_internal_ssl_flag_entry
//   struct: one verification flag and its phrase.
struct d_internal_ssl_flag_entry
{
    uint32_t    flag;
    const char* name;
};

// d_internal_ssl_flags
//   table: every assigned flag, in bit order, which is the order
// d_ssl_verify_describe lists them in.
static const struct d_internal_ssl_flag_entry d_internal_ssl_flags[] =
{
    { D_SSL_VERIFY_FLAG_NOT_PERFORMED,     "not performed"     },
    { D_SSL_VERIFY_FLAG_NO_CERTIFICATE,    "no certificate"    },
    { D_SSL_VERIFY_FLAG_UNTRUSTED,         "untrusted issuer"  },
    { D_SSL_VERIFY_FLAG_SELF_SIGNED,       "self-signed"       },
    { D_SSL_VERIFY_FLAG_EXPIRED,           "expired"           },
    { D_SSL_VERIFY_FLAG_NOT_YET_VALID,     "not yet valid"     },
    { D_SSL_VERIFY_FLAG_REVOKED,           "revoked"           },
    { D_SSL_VERIFY_FLAG_BAD_SIGNATURE,     "bad signature"     },
    { D_SSL_VERIFY_FLAG_WRONG_PURPOSE,     "wrong purpose"     },
    { D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH, "hostname mismatch" },
    { D_SSL_VERIFY_FLAG_PIN_MISMATCH,      "pin mismatch"      },
    { D_SSL_VERIFY_FLAG_OTHER,             "other"             }
};

// D_INTERNAL_SSL_FLAG_COUNT
//   constant: the number of entries in d_internal_ssl_flags.
#define D_INTERNAL_SSL_FLAG_COUNT                                              \
    (sizeof(d_internal_ssl_flags) / sizeof(d_internal_ssl_flags[0]))

/*
d_internal_ssl_flags_known
  The union of every assigned flag bit.
*/
static uint32_t
d_internal_ssl_flags_known(
    void
)
{
    uint32_t known = 0;

    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        known |= d_internal_ssl_flags[i].flag;
    }

    return known;
}

/*
d_ssl_verify_mode_name
  "none", "optional", or "required".
*/
const char*
d_ssl_verify_mode_name(
    enum d_ssl_verify_mode _mode
)
{
    switch (_mode)
    {
        case D_SSL_VERIFY_NONE:     return "none";
        case D_SSL_VERIFY_OPTIONAL: return "optional";
        case D_SSL_VERIFY_REQUIRED: return "required";
    }

    return NULL;
}

/*
d_ssl_verify_flag_name
  Only a single assigned bit has a name.
*/
const char*
d_ssl_verify_flag_name(
    uint32_t _flag
)
{
    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        if (d_internal_ssl_flags[i].flag == _flag)
        {
            return d_internal_ssl_flags[i].name;
        }
    }

    return NULL;
}

/*
d_internal_ssl_verify_measure
  The length of the description of an already folded set: "ok" for none, else
each phrase, with a separator before all but the first.
*/
static size_t
d_internal_ssl_verify_measure(
    uint32_t _flags
)
{
    size_t size = (_flags == 0) ? 2 : 0;

    // each phrase, and a separator before all but the first
    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        if ((_flags & d_internal_ssl_flags[i].flag) != 0)
        {
            if (size > 0)
            {
                size += 2;
            }

            size += strlen(d_internal_ssl_flags[i].name);
        }
    }

    return size;
}

/*
d_internal_ssl_verify_write
  Writes the description d_internal_ssl_verify_measure measured, in the same
order, and its terminator; the caller has checked that both fit.
*/
static void
d_internal_ssl_verify_write(
    uint32_t _flags,
    char*    _out
)
{
    size_t pos = 0;

    if (_flags == 0)
    {
        d_internal_ssl_put(_out,
                           &pos,
                           "ok",
                           2);
    }

    // each phrase, and a separator before all but the first
    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        if ((_flags & d_internal_ssl_flags[i].flag) != 0)
        {
            if (pos > 0)
            {
                d_internal_ssl_put(_out,
                                   &pos,
                                   ", ",
                                   2);
            }

            d_internal_ssl_put(_out,
                               &pos,
                               d_internal_ssl_flags[i].name,
                               strlen(d_internal_ssl_flags[i].name));
        }
    }

    _out[pos] = '\0';

    return;
}

/*
d_ssl_verify_describe
  Unassigned bits fold into OTHER first, so a result never lists "other" twice.
The measure and the write walk the same folded set, so they cannot disagree.
*/
enum d_ssl_error
d_ssl_verify_describe(
    uint32_t _flags,
    char*    _out,
    size_t   _capacity,
    size_t*  _out_size
)
{
    uint32_t         known  = d_internal_ssl_flags_known();
    uint32_t         flags  = _flags & known;
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if (!d_internal_ssl_out_args_ok(_out,
                                    _capacity,
                                    _out_size))
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if ((_flags & ~known) != 0)
    {
        flags |= D_SSL_VERIFY_FLAG_OTHER;
    }

    status = d_internal_ssl_out_check(_out,
                                      _capacity,
                                      d_internal_ssl_verify_measure(flags),
                                      1,
                                      _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_SSL_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    d_internal_ssl_verify_write(flags,
                                _out);

    return D_SSL_OK;
}

//==============================================================================
// 6.  IDENTITIES
//==============================================================================

// D_INTERNAL_SSL_PEER_NAMES_MAX
//   constant: the most subjectAltName entries the kernel asks an engine for.
// Real certificates carry at most a few hundred; the bound only stops a
// faulty engine from looping forever.
#define D_INTERNAL_SSL_PEER_NAMES_MAX 4096

/*
d_internal_ssl_is_label_char
  Letters, digits, and hyphens -- the LDH rule -- plus the underscore that
deployed names carry.
*/
static bool
d_internal_ssl_is_label_char(
    char _c
)
{
    return ( (d_net_ascii_is_alnum(_c)) ||
             (_c == '-')                ||
             (_c == '_') );
}

/*
d_internal_ssl_name_ok
  A valid host name with no trailing dot: the form every name takes once
d_internal_ssl_strip_dot has run. Testing a stripped name with
d_ssl_hostname_is_valid alone would let a second trailing dot through.
*/
static bool
d_internal_ssl_name_ok(
    struct d_pack_text _name
)
{
    return ( (_name.length > 0)                       &&
             (_name.data[_name.length - 1] != '.')    &&
             (d_ssl_hostname_is_valid(_name)) );
}

/*
d_internal_ssl_label_count
  The number of labels in a name already known to be valid and undotted.
*/
static size_t
d_internal_ssl_label_count(
    struct d_pack_text _name
)
{
    size_t count = 1;

    for (size_t i = 0; i < _name.length; ++i)
    {
        if (_name.data[i] == '.')
        {
            ++count;
        }
    }

    return count;
}

/*
d_internal_ssl_ipv4_parse
  Strict dotted-quad: four fields of one to three digits, each at most 255,
without leading zeros. inet_aton() reads "010" as octal 8, so a literal that
different parsers would read as different addresses is refused outright.
*/
static bool
d_internal_ssl_ipv4_parse(
    const char* _text,
    size_t      _length,
    uint8_t     _out[4]
)
{
    size_t pos = 0;

    for (size_t field = 0; field < 4; ++field)
    {
        size_t   start = pos;
        unsigned value = 0;

        // up to three digits; a fourth is caught by the separator test
        while ( (pos < _length)     &&
                (pos - start < 3)   &&
                (d_net_ascii_is_digit(_text[pos])) )
        {
            value = (value * 10u) + (unsigned)(_text[pos] - '0');
            ++pos;
        }

        if ( (pos == start)          ||
             (value > 255u)          ||
             ( (pos - start > 1) &&
               (_text[start] == '0') ) )
        {
            return false;
        }

        _out[field] = (uint8_t)value;

        // fields are joined by single dots
        if (field < 3)
        {
            if ( (pos >= _length) ||
                 (_text[pos] != '.') )
            {
                return false;
            }

            ++pos;
        }
    }

    return (pos == _length);
}

// d_internal_ssl_ipv6_text
//   struct: an IPv6 literal being read: the text and the cursor, the groups
// collected so far, and where "::" stood among them.
struct d_internal_ssl_ipv6_text
{
    const char* text;
    size_t      length;
    size_t      pos;
    uint16_t    groups[8];
    size_t      count;
    size_t      gap;
    bool        has_gap;
};

/*
d_internal_ssl_ipv6_group
  Reads the field at the cursor: one to four hex digits, or a dotted-quad
tail, which counts as two groups and so needs room for both. Sets `_last`
when the field ends the text, as a tail always does.
*/
static bool
d_internal_ssl_ipv6_group(
    struct d_internal_ssl_ipv6_text* _ip,
    bool*                            _last
)
{
    const size_t start = _ip->pos;
    unsigned     value = 0;

    // one to four hex digits
    while ( (_ip->pos < _ip->length)                    &&
            (_ip->pos - start < 4)                      &&
            (d_net_hex_value(_ip->text[_ip->pos]) >= 0) )
    {
        value = (value << 4) |
                (unsigned)d_net_hex_value(_ip->text[_ip->pos]);
        ++_ip->pos;
    }

    // a dot makes this field a dotted-quad tail, which ends the text
    if ( (_ip->pos < _ip->length) &&
         (_ip->text[_ip->pos] == '.') )
    {
        uint8_t quad[4] = { 0 };

        if ( (_ip->count > 6) ||
             (!d_internal_ssl_ipv4_parse(_ip->text + start,
                                         _ip->length - start,
                                         quad)) )
        {
            return false;
        }

        _ip->groups[_ip->count++] = (uint16_t)((quad[0] << 8) | quad[1]);
        _ip->groups[_ip->count++] = (uint16_t)((quad[2] << 8) | quad[3]);
        *_last                    = true;

        return true;
    }

    // an empty group, a fifth digit, or a ninth group
    if ( (_ip->pos == start)                             ||
         ( (_ip->pos < _ip->length) &&
           (d_net_hex_value(_ip->text[_ip->pos]) >= 0) ) ||
         (_ip->count == 8) )
    {
        return false;
    }

    _ip->groups[_ip->count++] = (uint16_t)value;
    *_last                    = (_ip->pos == _ip->length);

    return true;
}

/*
d_internal_ssl_ipv6_separator
  Reads what joins two groups: one colon, or, once, "::", whose place among
the groups is remembered. A text may not end on a lone colon.
*/
static bool
d_internal_ssl_ipv6_separator(
    struct d_internal_ssl_ipv6_text* _ip
)
{
    if (_ip->text[_ip->pos] != ':')
    {
        return false;
    }

    ++_ip->pos;

    if (_ip->pos == _ip->length)
    {
        return false;
    }

    // a second colon is the gap, which may stand only once
    if (_ip->text[_ip->pos] == ':')
    {
        if (_ip->has_gap)
        {
            return false;
        }

        _ip->has_gap = true;
        _ip->gap     = _ip->count;
        ++_ip->pos;
    }

    return true;
}

/*
d_internal_ssl_ipv6_store
  Without "::" all eight groups are spelled; with it, at most seven, as "::"
stands for at least one. The groups after the gap move to the tail, the gap
becomes zeros, and the groups are written out, high octet first.
*/
static bool
d_internal_ssl_ipv6_store(
    struct d_internal_ssl_ipv6_text* _ip,
    uint8_t                          _out[16]
)
{
    if ( ( (!_ip->has_gap) &&
           (_ip->count != 8) ) ||
         ( (_ip->has_gap) &&
           (_ip->count > 7) ) )
    {
        return false;
    }

    // the groups after the gap move to the tail; the gap becomes zeros
    if (_ip->has_gap)
    {
        const size_t tail = _ip->count - _ip->gap;

        memmove(&_ip->groups[8 - tail],
                &_ip->groups[_ip->gap],
                tail * sizeof(_ip->groups[0]));

        for (size_t i = _ip->gap; i < 8 - tail; ++i)
        {
            _ip->groups[i] = 0;
        }
    }

    for (size_t i = 0; i < 8; ++i)
    {
        _out[2 * i]       = (uint8_t)(_ip->groups[i] >> 8);
        _out[(2 * i) + 1] = (uint8_t)(_ip->groups[i] & 0xFFu);
    }

    return true;
}

/*
d_internal_ssl_ipv6_parse
  RFC 4291 section 2.2 text forms. Groups are collected in order, remembering
where the "::" stood; at the end, the groups after it slide to the tail and
the gap fills with zeros. "::" stands for at least one group, so a text that
spells out eight cannot also carry one. A dotted-quad tail counts as two
groups and may only come last.
*/
static bool
d_internal_ssl_ipv6_parse(
    const char* _text,
    size_t      _length,
    uint8_t     _out[16]
)
{
    struct d_internal_ssl_ipv6_text ip = { .text   = _text,
                                           .length = _length };

    // the shortest literal is "::", and only "::" may open with a colon
    if (_length < 2)
    {
        return false;
    }

    if (_text[0] == ':')
    {
        if (_text[1] != ':')
        {
            return false;
        }

        ip.has_gap = true;
        ip.pos     = 2;
    }

    // a group, then a separator, until a group ends the text
    while (ip.pos < _length)
    {
        bool last = false;

        if (!d_internal_ssl_ipv6_group(&ip,
                                       &last))
        {
            return false;
        }

        if (last)
        {
            break;
        }

        if (!d_internal_ssl_ipv6_separator(&ip))
        {
            return false;
        }
    }

    return d_internal_ssl_ipv6_store(&ip,
                                     _out);
}

/*
d_internal_ssl_peer_name_matches
  One presented name against the reference host, by kind: an address matches
only an iPAddress entry, and a name only a dNSName entry.
*/
static bool
d_internal_ssl_peer_name_matches(
    const struct d_ssl_peer_name* _name,
    struct d_pack_text            _host,
    bool                          _host_is_ip
)
{
    if (_host_is_ip)
    {
        return ( (_name->kind == D_SSL_NAME_IP) &&
                 (d_ssl_ip_match(_name->value,
                                 _host)) );
    }

    return ( (_name->kind == D_SSL_NAME_DNS)          &&
             (d_internal_ssl_bytes_ok(_name->value))  &&
             (d_ssl_hostname_match(
                  d_internal_ssl_text((const char*)_name->value.data,
                                      _name->value.size),
                  _host)) );
}

/*
d_ssl_hostname_is_valid
  One pass over the labels. One trailing dot is stripped before the length
test, since RFC 1035's 253 counts the name without it.
*/
bool
d_ssl_hostname_is_valid(
    struct d_pack_text _host
)
{
    struct d_pack_text host  = { NULL, 0 };
    size_t             start = 0;

    // parameter validation
    if (!d_internal_ssl_text_ok(_host))
    {
        return false;
    }

    host = d_internal_ssl_strip_dot(_host);

    if ( (host.length == 0) ||
         (host.length > D_SSL_HOSTNAME_MAX) )
    {
        return false;
    }

    // a label ends at a dot or at the end of the name
    for (size_t i = 0; i <= host.length; ++i)
    {
        if ( (i < host.length) &&
             (host.data[i] != '.') )
        {
            if (!d_internal_ssl_is_label_char(host.data[i]))
            {
                return false;
            }

            continue;
        }

        if ( (i == start)                    ||
             (i - start > D_SSL_LABEL_MAX)   ||
             (host.data[start] == '-')       ||
             (host.data[i - 1] == '-') )
        {
            return false;
        }

        start = i + 1;
    }

    return true;
}

/*
d_ssl_host_is_ip
  Whatever d_ssl_ip_parse accepts.
*/
bool
d_ssl_host_is_ip(
    struct d_pack_text _host
)
{
    uint8_t address[D_SSL_IP_MAX] = { 0 };
    size_t  length                = 0;

    return (d_ssl_ip_parse(_host,
                           address,
                           &length) == D_SSL_OK);
}

/*
d_ssl_ip_parse
  A colon anywhere means IPv6; otherwise the text must be a dotted quad. The
parse goes through scratch storage, so a failure leaves _out untouched.
*/
enum d_ssl_error
d_ssl_ip_parse(
    struct d_pack_text _text,
    uint8_t            _out[D_SSL_IP_MAX],
    size_t*            _out_length
)
{
    uint8_t address[D_SSL_IP_MAX] = { 0 };
    bool    ipv6                  = false;
    bool    parsed                = false;

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_text)) ||
         (_out == NULL)                   ||
         (_out_length == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if ( (_text.length == 0) ||
         (_text.length > D_SSL_IP_TEXT_MAX) )
    {
        return D_SSL_ERROR_MALFORMED;
    }

    ipv6   = (memchr(_text.data,
                     ':',
                     _text.length) != NULL);
    parsed = ipv6 ? d_internal_ssl_ipv6_parse(_text.data,
                                              _text.length,
                                              address)
                  : d_internal_ssl_ipv4_parse(_text.data,
                                              _text.length,
                                              address);

    if (!parsed)
    {
        return D_SSL_ERROR_MALFORMED;
    }

    *_out_length = ipv6 ? 16u : 4u;
    memcpy(_out,
           address,
           *_out_length);

    return D_SSL_OK;
}

/*
d_ssl_sni_from_host
  The address test runs on the stripped form, so "192.0.2.1." is still an
address; the name test runs on the original, which may carry the one trailing
dot the stripped form has lost.
*/
enum d_ssl_error
d_ssl_sni_from_host(
    struct d_pack_text  _host,
    struct d_pack_text* _out_sni
)
{
    struct d_pack_text host = { NULL, 0 };

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_host)) ||
         (_out_sni == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    host = d_internal_ssl_strip_dot(_host);

    // RFC 6066 section 3: address literals are not server names
    if (d_ssl_host_is_ip(host))
    {
        return D_SSL_ERROR_NOT_FOUND;
    }

    if (!d_ssl_hostname_is_valid(_host))
    {
        return D_SSL_ERROR_MALFORMED;
    }

    *_out_sni = host;

    return D_SSL_OK;
}

/*
d_ssl_hostname_match
  RFC 9525 section 6.3. The reference must be a real host name, never an
address. A presented name either equals it, ignoring case, or is "*." over a
suffix of two or more labels that equals everything after the reference's
first label. Every other placement of '*' fails d_internal_ssl_name_ok, since
'*' is not a label character.
*/
bool
d_ssl_hostname_match(
    struct d_pack_text _pattern,
    struct d_pack_text _host
)
{
    struct d_pack_text pattern = { NULL, 0 };
    struct d_pack_text host    = { NULL, 0 };

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_pattern)) ||
         (!d_internal_ssl_text_ok(_host)) )
    {
        return false;
    }

    pattern = d_internal_ssl_strip_dot(_pattern);
    host    = d_internal_ssl_strip_dot(_host);

    // the reference is a name: an address never matches a DNS-ID
    if ( (!d_internal_ssl_name_ok(host)) ||
         (d_ssl_host_is_ip(host)) )
    {
        return false;
    }

    // a wildcard is the whole left-most label, over at least two more
    if ( (pattern.length > 2)      &&
         (pattern.data[0] == '*')  &&
         (pattern.data[1] == '.') )
    {
        struct d_pack_text suffix = d_internal_ssl_text(pattern.data + 2,
                                                         pattern.length - 2);
        const char*        dot    = (const char*)memchr(host.data,
                                                        '.',
                                                        host.length);
        size_t             skip   = 0;

        if ( (dot == NULL)                            ||
             (!d_internal_ssl_name_ok(suffix))        ||
             (d_internal_ssl_label_count(suffix) < 2) )
        {
            return false;
        }

        // the wildcard stands in for exactly the reference's first label
        skip = (size_t)(dot - host.data) + 1;

        return d_net_ascii_ieq(suffix,
                               d_internal_ssl_text(host.data + skip,
                                                   host.length - skip));
    }

    return ( (d_internal_ssl_name_ok(pattern)) &&
             (d_net_ascii_ieq(pattern,
                              host)) );
}

/*
d_ssl_ip_match
  Octet for octet, after parsing the reference; lengths must agree, so an IPv4
reference never matches an IPv4-mapped IPv6 entry.
*/
bool
d_ssl_ip_match(
    struct d_pack_bytes _presented,
    struct d_pack_text  _host
)
{
    uint8_t address[D_SSL_IP_MAX] = { 0 };
    size_t  length                = 0;

    // parameter validation
    if ( (!d_internal_ssl_bytes_ok(_presented)) ||
         (d_ssl_ip_parse(_host,
                         address,
                         &length) != D_SSL_OK) )
    {
        return false;
    }

    return ( (_presented.size == length) &&
             (memcmp(_presented.data,
                     address,
                     length) == 0) );
}

/*
d_ssl_identity_match
  Decides once whether the reference is an address, then tries each entry.
*/
bool
d_ssl_identity_match(
    const struct d_ssl_peer_name* _names,
    size_t                        _count,
    struct d_pack_text            _host
)
{
    bool is_ip = false;

    // parameter validation
    if ( ( (_names == NULL) &&
           (_count > 0) )                 ||
         (!d_internal_ssl_text_ok(_host)) )
    {
        return false;
    }

    is_ip = d_ssl_host_is_ip(_host);

    for (size_t i = 0; i < _count; ++i)
    {
        if (d_internal_ssl_peer_name_matches(&_names[i],
                                             _host,
                                             is_ip))
        {
            return true;
        }
    }

    return false;
}

//==============================================================================
// 7.  ALPN
//==============================================================================

/*
d_ssl_alpn_is_valid
  Walks the entries; the walk must land exactly on the end of the span.
*/
bool
d_ssl_alpn_is_valid(
    struct d_pack_bytes _list
)
{
    const uint8_t* data   = (const uint8_t*)_list.data;
    size_t         offset = 0;

    // parameter validation, and the bounds on a whole list
    if ( (!d_internal_ssl_bytes_ok(_list)) ||
         (_list.size < 2)                  ||
         (_list.size > D_SSL_ALPN_LIST_MAX) )
    {
        return false;
    }

    while (offset < _list.size)
    {
        size_t length = data[offset];

        if ( (length == 0) ||
             (length > _list.size - offset - 1) )
        {
            return false;
        }

        offset += 1 + length;
    }

    return true;
}

/*
d_ssl_alpn_contains
  Exact, case-sensitive comparison: protocol names are opaque octets.
*/
bool
d_ssl_alpn_contains(
    struct d_pack_bytes _list,
    struct d_pack_text  _protocol
)
{
    size_t             offset   = 0;
    struct d_pack_text protocol = { NULL, 0 };

    // parameter validation
    if ( (!d_ssl_alpn_is_valid(_list))           ||
         (!d_internal_ssl_text_ok(_protocol))    ||
         (_protocol.length == 0) )
    {
        return false;
    }

    while (d_ssl_alpn_next(_list,
                           &offset,
                           &protocol) == D_SSL_OK)
    {
        if ( (protocol.length == _protocol.length) &&
             (memcmp(protocol.data,
                     _protocol.data,
                     protocol.length) == 0) )
        {
            return true;
        }
    }

    return false;
}

/*
d_ssl_alpn_encode
  Measure, check, write, as every two-call producer here. The whole list is
bounded during the measure, so an oversized list is refused before any byte
is written.
*/
enum d_ssl_error
d_ssl_alpn_encode(
    const struct d_pack_text* _protocols,
    size_t                    _count,
    void*                     _out,
    size_t                    _capacity,
    size_t*                   _out_size
)
{
    unsigned char*   out    = (unsigned char*)_out;
    size_t           size   = 0;
    size_t           pos    = 0;
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if ( (_protocols == NULL) ||
         (_count == 0)        ||
         (!d_internal_ssl_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    // measure, refusing any name the wire format cannot carry
    for (size_t i = 0; i < _count; ++i)
    {
        if (!d_internal_ssl_text_ok(_protocols[i]))
        {
            return D_SSL_ERROR_INVALID_ARGUMENT;
        }

        if ( (_protocols[i].length == 0) ||
             (_protocols[i].length > D_SSL_ALPN_PROTOCOL_MAX) )
        {
            return D_SSL_ERROR_MALFORMED;
        }

        size += 1 + _protocols[i].length;

        if (size > D_SSL_ALPN_LIST_MAX)
        {
            return D_SSL_ERROR_MALFORMED;
        }
    }

    status = d_internal_ssl_out_check(_out,
                                      _capacity,
                                      size,
                                      0,
                                      _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_SSL_OK) ||
         (out == NULL) )
    {
        return status;
    }

    for (size_t i = 0; i < _count; ++i)
    {
        out[pos++] = (unsigned char)_protocols[i].length;
        memcpy(out + pos,
               _protocols[i].data,
               _protocols[i].length);
        pos += _protocols[i].length;
    }

    return D_SSL_OK;
}

/*
d_ssl_alpn_next
  One entry per call. The bounds test is written as a subtraction from the
remaining length, so it cannot overflow however large the offset.
*/
enum d_ssl_error
d_ssl_alpn_next(
    struct d_pack_bytes _list,
    size_t*             _offset,
    struct d_pack_text* _out_protocol
)
{
    const char* data   = (const char*)_list.data;
    size_t      length = 0;

    // parameter validation
    if ( (!d_internal_ssl_bytes_ok(_list)) ||
         (_offset == NULL)                 ||
         (_out_protocol == NULL)           ||
         (*_offset > _list.size) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if (*_offset == _list.size)
    {
        return D_SSL_ERROR_NOT_FOUND;
    }

    length = (unsigned char)data[*_offset];

    // an entry is a length octet and that many name octets, never zero
    if ( (length == 0) ||
         (length > _list.size - *_offset - 1) )
    {
        return D_SSL_ERROR_MALFORMED;
    }

    *_out_protocol = d_internal_ssl_text(data + *_offset + 1,
                                         length);
    *_offset      += 1 + length;

    return D_SSL_OK;
}

/*
d_ssl_alpn_select
  Both lists are validated whole before either is trusted, which is exactly
the check CVE-2024-5535's selection routine skipped. The server's order
decides (RFC 7301 section 3.2), and the answer points into the client's list.
*/
enum d_ssl_error
d_ssl_alpn_select(
    struct d_pack_bytes _preferred,
    struct d_pack_bytes _offered,
    struct d_pack_text* _out_protocol
)
{
    size_t             preferred_offset = 0;
    struct d_pack_text preferred        = { NULL, 0 };

    // parameter validation
    if (_out_protocol == NULL)
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if ( (!d_ssl_alpn_is_valid(_preferred)) ||
         (!d_ssl_alpn_is_valid(_offered)) )
    {
        return D_SSL_ERROR_MALFORMED;
    }

    while (d_ssl_alpn_next(_preferred,
                           &preferred_offset,
                           &preferred) == D_SSL_OK)
    {
        size_t             offered_offset = 0;
        struct d_pack_text offered        = { NULL, 0 };

        while (d_ssl_alpn_next(_offered,
                               &offered_offset,
                               &offered) == D_SSL_OK)
        {
            if ( (offered.length == preferred.length) &&
                 (memcmp(offered.data,
                         preferred.data,
                         offered.length) == 0) )
            {
                *_out_protocol = offered;

                return D_SSL_OK;
            }
        }
    }

    return D_SSL_ERROR_NOT_FOUND;
}

//==============================================================================
// 8.  SNIFFING
//==============================================================================

/*
d_internal_ssl_sniff_ssl2
  The SSL 2.0 form: a two-byte length with its top bit set, message type 1
(CLIENT-HELLO), then a version of 0x0002 or 0x03xx.
*/
static enum d_ssl_sniff_result
d_internal_ssl_sniff_ssl2(
    const uint8_t* _data,
    size_t         _size
)
{
    // a record of length zero carries no ClientHello
    if ( (_size >= 2)                  &&
         ((_data[0] & 0x7Fu) == 0)     &&
         (_data[1] == 0) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if ( (_size >= 3) &&
         (_data[2] != 1) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if ( (_size >= 4) &&
         (_data[3] != 0) &&
         (_data[3] != 3) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if (_size < D_SSL_RECORD_HEADER_SIZE)
    {
        return D_SSL_SNIFF_NEED_MORE;
    }

    if ( ( (_data[3] == 0) &&
           (_data[4] != 2) ) ||
         ( (_data[3] == 3) &&
           (_data[4] > 4) ) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    return D_SSL_SNIFF_SSL2;
}

/*
d_ssl_sniff
  Each byte narrows the verdict as it arrives. A TLS stream opens with a
handshake record or, from a server refusing a ClientHello, an alert; the
version's major byte is 3 and its minor at most 4, and the length lies within
the record limit. A plaintext greeting fails on its first byte, since no
printable character is a handshake or alert content type.
*/
enum d_ssl_sniff_result
d_ssl_sniff(
    struct d_pack_bytes         _prefix,
    struct d_ssl_record_header* _out_header
)
{
    const uint8_t* data   = (const uint8_t*)_prefix.data;
    uint16_t       length = 0;

    // an empty or unusable prefix says nothing yet
    if ( (_prefix.size == 0) ||
         (data == NULL) )
    {
        return D_SSL_SNIFF_NEED_MORE;
    }

    if ((data[0] & 0x80u) != 0)
    {
        return d_internal_ssl_sniff_ssl2(data,
                                         _prefix.size);
    }

    if ( (data[0] != D_SSL_CONTENT_HANDSHAKE) &&
         (data[0] != D_SSL_CONTENT_ALERT) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if ( ( (_prefix.size >= 2) &&
           (data[1] != 3) )      ||
         ( (_prefix.size >= 3) &&
           (data[2] > 4) ) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if (_prefix.size < D_SSL_RECORD_HEADER_SIZE)
    {
        return D_SSL_SNIFF_NEED_MORE;
    }

    length = (uint16_t)((data[3] << 8) | data[4]);

    if ( (length == 0) ||
         (length > D_SSL_RECORD_CIPHERTEXT_MAX) )
    {
        return D_SSL_SNIFF_NOT_TLS;
    }

    if (_out_header != NULL)
    {
        _out_header->content_type = data[0];
        _out_header->version      = (uint16_t)((data[1] << 8) | data[2]);
        _out_header->length       = length;
    }

    return D_SSL_SNIFF_TLS;
}

//==============================================================================
// 9.  PEM
//==============================================================================

/*
d_internal_ssl_pem_label_ok
  RFC 7468 section 3: printable characters, where a hyphen or a space may only
separate two others. A label running into a line break means the BEGIN line
lacked its closing dashes.
*/
static bool
d_internal_ssl_pem_label_ok(
    struct d_pack_text _label
)
{
    for (size_t i = 0; i < _label.length; ++i)
    {
        char c         = _label.data[i];
        bool separator = ( (c == '-') ||
                           (c == ' ') );

        if ( (c < 0x20) ||
             (c > 0x7E) )
        {
            return false;
        }

        // separators stand singly, between two label characters
        if ( (separator) &&
             ( (i == 0)                         ||
               (i + 1 == _label.length)         ||
               (_label.data[i + 1] == '-')      ||
               (_label.data[i + 1] == ' ') ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_ssl_pem_body_start
  Where a block's body starts: past any blanks after the dashes that close the
BEGIN line, and past that line's ending, CRLF or LF.
*/
static size_t
d_internal_ssl_pem_body_start(
    struct d_pack_text _text,
    size_t             _pos
)
{
    while ( (_pos < _text.length) &&
            (d_net_ascii_is_blank(_text.data[_pos])) )
    {
        ++_pos;
    }

    if ( (_pos < _text.length) &&
         (_text.data[_pos] == '\r') )
    {
        ++_pos;
    }

    if ( (_pos < _text.length) &&
         (_text.data[_pos] == '\n') )
    {
        ++_pos;
    }

    return _pos;
}

/*
d_internal_ssl_pem_end
  Where the END line of the block whose body starts at `_body_start` begins.
The next boundary must be that line, with the block's label: a second BEGIN
first, another label, or no END at all is malformed, and gives the text's
length, which no END line can start at.
*/
static size_t
d_internal_ssl_pem_end(
    struct d_pack_text _text,
    size_t             _body_start,
    struct d_pack_text _label
)
{
    const size_t end   = d_internal_ssl_find(_text,
                                             _body_start,
                                             "-----END ");
    const size_t after = end + 9 + _label.length + 5;

    if ( (end == _text.length)                      ||
         (d_internal_ssl_find(_text,
                              _body_start,
                              "-----BEGIN ") < end) ||
         (after > _text.length)                     ||
         (memcmp(_text.data + end + 9,
                 _label.data,
                 _label.length) != 0)               ||
         (memcmp(_text.data + end + 9 + _label.length,
                 "-----",
                 5) != 0) )
    {
        return _text.length;
    }

    return end;
}

/*
d_ssl_pem_next
  Finds a BEGIN line, reads its label, and requires that the next boundary be
the END line with the same label: a second BEGIN first, or a different label,
is malformed. On MALFORMED the cursor moves to the end, so a loop over a
damaged bundle terminates.
*/
enum d_ssl_error
d_ssl_pem_next(
    struct d_pack_text      _text,
    size_t*                 _offset,
    struct d_ssl_pem_block* _out
)
{
    size_t             begin       = 0;
    size_t             label_start = 0;
    size_t             label_end   = 0;
    size_t             body_start  = 0;
    size_t             end         = 0;
    struct d_pack_text label       = { NULL, 0 };

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_text)) ||
         (_offset == NULL)                ||
         (_out == NULL)                   ||
         (*_offset > _text.length) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    begin = d_internal_ssl_find(_text,
                                *_offset,
                                "-----BEGIN ");

    if (begin == _text.length)
    {
        *_offset = _text.length;

        return D_SSL_ERROR_NOT_FOUND;
    }

    // the label runs to the dashes that close the BEGIN line
    label_start = begin + 11;
    label_end   = d_internal_ssl_find(_text,
                                      label_start,
                                      "-----");
    label       = d_internal_ssl_text(_text.data + label_start,
                                      label_end - label_start);

    if ( (label_end == _text.length) ||
         (!d_internal_ssl_pem_label_ok(label)) )
    {
        *_offset = _text.length;

        return D_SSL_ERROR_MALFORMED;
    }

    // the body starts on the line after BEGIN, and runs to the matching END
    body_start = d_internal_ssl_pem_body_start(_text,
                                               label_end + 5);
    end        = d_internal_ssl_pem_end(_text,
                                        body_start,
                                        label);

    if (end == _text.length)
    {
        *_offset = _text.length;

        return D_SSL_ERROR_MALFORMED;
    }

    _out->label = label;
    _out->body  = d_internal_ssl_text(_text.data + body_start,
                                      end - body_start);
    *_offset    = end + 9 + label.length + 5;

    return D_SSL_OK;
}

/*
d_ssl_pem_decode
  A PEM body is base64 broken into lines, so the codec decodes it with blanks
allowed and padding required. The codec judges the whole body before writing,
and reports the size even for a buffer too small; a malformed body leaves
`*_out_size` as it was.
*/
enum d_ssl_error
d_ssl_pem_decode(
    struct d_pack_text _body,
    void*              _out,
    size_t             _capacity,
    size_t*            _out_size
)
{
    // parameter validation
    if ( (!d_internal_ssl_text_ok(_body)) ||
         (!d_internal_ssl_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    size_t                       size   = 0;
    const enum d_net_codec_error status =
        d_net_base64_decode(_body,
                            D_NET_BASE64_BLANKS,
                            _out,
                            _capacity,
                            &size);

    // a malformed body reports nothing; a decoded or measured one its size
    switch (status)
    {
        case D_NET_CODEC_OK:
            *_out_size = size;

            return D_SSL_OK;

        case D_NET_CODEC_ERROR_BUFFER_TOO_SMALL:
            *_out_size = size;

            return D_SSL_ERROR_BUFFER_TOO_SMALL;

        case D_NET_CODEC_ERROR_MALFORMED:
            return D_SSL_ERROR_MALFORMED;

        default:
            return D_SSL_ERROR_INVALID_ARGUMENT;
    }
}

//==============================================================================
// 10.  FINGERPRINTS
//==============================================================================

D_STATIC_ASSERT(D_SSL_FINGERPRINT_LENGTH == D_NET_DIGEST_SHA256_SIZE,
                "ssl: a certificate fingerprint is a SHA-256 digest");

/*
d_ssl_fingerprint_equal
  d_net_digest_equal reads every octet whatever the first difference, so the
time taken says nothing about how much of a pin matched.
*/
bool
d_ssl_fingerprint_equal(
    const uint8_t _a[D_SSL_FINGERPRINT_LENGTH],
    const uint8_t _b[D_SSL_FINGERPRINT_LENGTH]
)
{
    return d_net_digest_equal(_a,
                              _b,
                              D_SSL_FINGERPRINT_LENGTH);
}

/*
d_ssl_fingerprint_compute
  SHA-256 over the DER, exactly as certificate tools compute it.
*/
enum d_ssl_error
d_ssl_fingerprint_compute(
    struct d_pack_bytes _der,
    uint8_t             _out[D_SSL_FINGERPRINT_LENGTH]
)
{
    // parameter validation
    if ( (!d_internal_ssl_bytes_ok(_der)) ||
         (_out == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    // the digest refuses only what the validation above already has
    const bool computed = d_net_digest_compute(D_NET_DIGEST_SHA256,
                                               _der,
                                               _out);

    return (computed) ? D_SSL_OK
                      : D_SSL_ERROR_INVALID_ARGUMENT;
}

/*
d_ssl_fingerprint_format
  Uppercase pairs with colons between them, and a NUL.
*/
enum d_ssl_error
d_ssl_fingerprint_format(
    const uint8_t _fingerprint[D_SSL_FINGERPRINT_LENGTH],
    char          _out[D_SSL_FINGERPRINT_TEXT_LENGTH + 1]
)
{
    size_t pos = 0;

    // parameter validation
    if ( (_fingerprint == NULL) ||
         (_out == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    // each octet as two digits, a colon before all but the first
    for (size_t i = 0; i < D_SSL_FINGERPRINT_LENGTH; ++i)
    {
        if (i > 0)
        {
            _out[pos++] = ':';
        }

        _out[pos++] = d_net_hex_digit((unsigned int)(_fingerprint[i] >> 4),
                                      D_NET_HEX_UPPER);
        _out[pos++] = d_net_hex_digit((unsigned int)(_fingerprint[i] & 0x0fu),
                                      D_NET_HEX_UPPER);
    }

    _out[pos] = '\0';

    return D_SSL_OK;
}

/*
d_ssl_fingerprint_parse
  Tool output is cut at its last '=' first; what remains must be exactly 64
contiguous digits or 32 colon-joined pairs, so a truncated or padded paste is
refused rather than half-read.
*/
enum d_ssl_error
d_ssl_fingerprint_parse(
    struct d_pack_text _text,
    uint8_t            _out[D_SSL_FINGERPRINT_LENGTH]
)
{
    uint8_t            digest[D_SSL_FINGERPRINT_LENGTH] = { 0 };
    struct d_pack_text text                             = { NULL, 0 };
    size_t             stride                           = 0;

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_text)) ||
         (_out == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    text = d_net_ascii_trim_space(_text);

    // "sha256 Fingerprint=AB:CD:..." keeps only what follows the '='
    for (size_t i = text.length; i > 0; --i)
    {
        if (text.data[i - 1] == '=')
        {
            text = d_net_ascii_trim_space(
                       d_internal_ssl_text(text.data + i,
                                              text.length - i));

            break;
        }
    }

    // contiguous digits, or pairs joined by colons
    if (text.length == 2 * D_SSL_FINGERPRINT_LENGTH)
    {
        stride = 2;
    }
    else if (text.length == D_SSL_FINGERPRINT_TEXT_LENGTH)
    {
        stride = 3;
    }
    else
    {
        return D_SSL_ERROR_MALFORMED;
    }

    for (size_t i = 0; i < D_SSL_FINGERPRINT_LENGTH; ++i)
    {
        size_t at   = i * stride;
        int    high = d_net_hex_value(text.data[at]);
        int    low  = d_net_hex_value(text.data[at + 1]);

        if ( (high < 0)                                  ||
             (low < 0)                                   ||
             ( (stride == 3)                          &&
               (i + 1 < D_SSL_FINGERPRINT_LENGTH)     &&
               (text.data[at + 2] != ':') ) )
        {
            return D_SSL_ERROR_MALFORMED;
        }

        digest[i] = (uint8_t)((high << 4) | low);
    }

    memcpy(_out,
           digest,
           sizeof(digest));

    return D_SSL_OK;
}

//==============================================================================
// 11.  TRUST STORES
//==============================================================================

// d_internal_ssl_ca_files
//   table: env_ssl.h's candidate CA bundle files, NULL-terminated.
static const char* const d_internal_ssl_ca_files[] =
{
    D_ENV_SSL_CA_FILES
    NULL
};

// d_internal_ssl_ca_dirs
//   table: env_ssl.h's candidate CA directories, NULL-terminated.
static const char* const d_internal_ssl_ca_dirs[] =
{
    D_ENV_SSL_CA_DIRS
    NULL
};

/*
d_ssl_ca_file_candidates
  The count excludes the terminating NULL.
*/
const char* const*
d_ssl_ca_file_candidates(
    size_t* _out_count
)
{
    if (_out_count != NULL)
    {
        *_out_count = (sizeof(d_internal_ssl_ca_files) /
                       sizeof(d_internal_ssl_ca_files[0])) - 1;
    }

    return d_internal_ssl_ca_files;
}

/*
d_ssl_ca_dir_candidates
  The count excludes the terminating NULL.
*/
const char* const*
d_ssl_ca_dir_candidates(
    size_t* _out_count
)
{
    if (_out_count != NULL)
    {
        *_out_count = (sizeof(d_internal_ssl_ca_dirs) /
                       sizeof(d_internal_ssl_ca_dirs[0])) - 1;
    }

    return d_internal_ssl_ca_dirs;
}

//==============================================================================
// 12.  CONFIGURATION
//==============================================================================

/*
d_internal_ssl_path_ok
  A path is absent (NULL) or names something; an empty string does neither and
would reach a library as "the current directory" or worse.
*/
static bool
d_internal_ssl_path_ok(
    const char* _path
)
{
    return ( (_path == NULL) ||
             (_path[0] != '\0') );
}

/*
d_internal_ssl_cipher_ok
  Cipher strings go to libraries verbatim. No library syntax uses control
characters or non-ASCII, so they are refused before they reach one.
*/
static bool
d_internal_ssl_cipher_ok(
    const char* _ciphers
)
{
    if (_ciphers == NULL)
    {
        return true;
    }

    if (_ciphers[0] == '\0')
    {
        return false;
    }

    for (const char* c = _ciphers; *c != '\0'; ++c)
    {
        if ( (*c < 0x20) ||
             (*c > 0x7E) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_ssl_versions_ok
  The version rules of d_ssl_config_validate. SSL 3.0 is UNSUPPORTED wherever
it appears; so is a legacy minimum, unless the build allows one. A zero or
unknown minimum, or a maximum below it, is CONFIG.
*/
static enum d_ssl_error
d_internal_ssl_versions_ok(
    const struct d_ssl_config* _config
)
{
    int minimum = (int)_config->min_version;
    int maximum = (int)_config->max_version;

    if ( (minimum == (int)D_SSL_VERSION_SSL3) ||
         (maximum == (int)D_SSL_VERSION_SSL3) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    if ( (!d_ssl_version_is_known(_config->min_version)) ||
         ( (maximum != (int)D_SSL_VERSION_NONE) &&
           ( (!d_ssl_version_is_known(_config->max_version)) ||
             (maximum < minimum) ) ) )
    {
        return D_SSL_ERROR_CONFIG;
    }

    // RFC 8996's deprecation holds unless the build opted out of it
    if ( (!D_INTERNAL_SSL_ALLOW_LEGACY) &&
         (minimum < (int)D_SSL_VERSION_TLS1_2) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    return D_SSL_OK;
}

/*
d_internal_ssl_identity_ok
  The local-identity rules of d_ssl_config_validate: a certificate and its key
come together, each from exactly one source, and a server has them.
*/
static bool
d_internal_ssl_identity_ok(
    const struct d_ssl_config* _config
)
{
    bool certificate_file = (_config->certificate_file != NULL);
    bool certificate_pem  = (_config->certificate_pem.length > 0);
    bool key_file         = (_config->private_key_file != NULL);
    bool key_pem          = (_config->private_key_pem.length > 0);
    bool has_certificate  = (certificate_file || certificate_pem);
    bool has_key          = (key_file || key_pem);

    return ( (!(certificate_file && certificate_pem)) &&
             (!(key_file && key_pem))                 &&
             (has_certificate == has_key)             &&
             ( (_config->role == D_SSL_ROLE_CLIENT) ||
               (has_certificate) ) );
}

/*
d_ssl_config_init
  Clears the record, then sets the defaults the header lists. Everything left
zero means "absent".
*/
void
d_ssl_config_init(
    struct d_ssl_config* _config,
    enum d_ssl_role      _role
)
{
    bool client = (_role == D_SSL_ROLE_CLIENT);

    if (_config == NULL)
    {
        return;
    }

    memset(_config,
           0,
           sizeof(*_config));

    _config->role             = _role;
    _config->min_version      = D_SSL_VERSION_FLOOR;
    _config->max_version      = D_SSL_VERSION_NONE;
    _config->verify           = client ? D_SSL_VERIFY_REQUIRED
                                       : D_SSL_VERIFY_NONE;
    _config->verify_hostname  = client;
    _config->use_system_trust = true;

    return;
}

/*
d_ssl_config_add_pin
  Appends; the pin table is fixed-size, so a full one is BUFFER_TOO_SMALL.
*/
enum d_ssl_error
d_ssl_config_add_pin(
    struct d_ssl_config* _config,
    const uint8_t        _fingerprint[D_SSL_FINGERPRINT_LENGTH]
)
{
    // parameter validation
    if ( (_config == NULL) ||
         (_fingerprint == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if (_config->pin_count >= D_SSL_PINS_MAX)
    {
        return D_SSL_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(_config->pins[_config->pin_count],
           _fingerprint,
           D_SSL_FINGERPRINT_LENGTH);
    ++_config->pin_count;

    return D_SSL_OK;
}

/*
d_internal_ssl_verify_ok
  The verification rules of d_ssl_config_validate: a known mode, OPTIONAL for
a server alone, and, when the chain is verified rather than pins alone,
something to anchor it.
*/
static bool
d_internal_ssl_verify_ok(
    const struct d_ssl_config* _config
)
{
    const bool verifies    = (_config->verify != D_SSL_VERIFY_NONE);
    const bool has_anchors = ( (_config->use_system_trust)   ||
                               (_config->ca_file != NULL)    ||
                               (_config->ca_path != NULL)    ||
                               (_config->ca_pem.length > 0) );

    // verification modes, and OPTIONAL being a server's alone
    if ( ( (_config->verify != D_SSL_VERIFY_NONE)     &&
           (_config->verify != D_SSL_VERIFY_OPTIONAL) &&
           (_config->verify != D_SSL_VERIFY_REQUIRED) ) ||
         ( (_config->verify == D_SSL_VERIFY_OPTIONAL) &&
           (_config->role == D_SSL_ROLE_CLIENT) ) )
    {
        return false;
    }

    // verifying against a chain needs something to anchor it
    if ( (verifies)            &&
         (!_config->pin_only)  &&
         (!has_anchors) )
    {
        return false;
    }

    return true;
}

/*
d_internal_ssl_alpn_pins_ok
  The ALPN and pinning rules of d_ssl_config_validate: a well-formed list,
present when a protocol is required; and pins that fit, that pin_only has,
and that are set only when the peer is verified.
*/
static bool
d_internal_ssl_alpn_pins_ok(
    const struct d_ssl_config* _config
)
{
    const bool verifies = (_config->verify != D_SSL_VERIFY_NONE);

    // an ALPN list must be well-formed, and a requirement needs a list
    if ( ( (_config->alpn.size > 0) &&
           (!d_ssl_alpn_is_valid(_config->alpn)) ) ||
         ( (_config->alpn_required) &&
           (_config->alpn.size == 0) ) )
    {
        return false;
    }

    // pins fit, pin_only has some, and they only mean something when verifying
    if ( (_config->pin_count > D_SSL_PINS_MAX)     ||
         ( (_config->pin_only) &&
           (_config->pin_count == 0) )             ||
         ( (_config->pin_count > 0) &&
           (!verifies) ) )
    {
        return false;
    }

    return true;
}

/*
d_ssl_config_validate
  The rules in the header, in its order. Spans that break their own contract
are caller bugs and INVALID_ARGUMENT; everything else is CONFIG,
except what the build refuses outright, which is UNSUPPORTED.
*/
enum d_ssl_error
d_ssl_config_validate(
    const struct d_ssl_config* _config
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if ( (_config == NULL)                                  ||
         (!d_internal_ssl_text_ok(_config->ca_pem))         ||
         (!d_internal_ssl_text_ok(_config->certificate_pem)) ||
         (!d_internal_ssl_text_ok(_config->private_key_pem)) ||
         (!d_internal_ssl_bytes_ok(_config->alpn)) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if ( (_config->role != D_SSL_ROLE_CLIENT) &&
         (_config->role != D_SSL_ROLE_SERVER) )
    {
        return D_SSL_ERROR_CONFIG;
    }

    status = d_internal_ssl_versions_ok(_config);

    if (status != D_SSL_OK)
    {
        return status;
    }

    // verification, identity, files and ciphers, then ALPN and pins
    if ( (!d_internal_ssl_verify_ok(_config))                       ||
         (!d_internal_ssl_identity_ok(_config))                     ||
         (!d_internal_ssl_path_ok(_config->ca_file))                ||
         (!d_internal_ssl_path_ok(_config->ca_path))                ||
         (!d_internal_ssl_path_ok(_config->certificate_file))       ||
         (!d_internal_ssl_path_ok(_config->private_key_file))       ||
         (!d_internal_ssl_cipher_ok(_config->cipher_list))          ||
         (!d_internal_ssl_cipher_ok(_config->cipher_suites))        ||
         (!d_internal_ssl_alpn_pins_ok(_config)) )
    {
        return D_SSL_ERROR_CONFIG;
    }

    // a key log leaks session secrets, so a build must have asked for it
    if ( (_config->keylog != NULL) &&
         (!D_INTERNAL_SSL_KEYLOG) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    return D_SSL_OK;
}

/*
d_ssl_config_plan
  The whole of the configuration's meaning for a handshake, derived in one
place so no engine derives it differently.
*/
enum d_ssl_error
d_ssl_config_plan(
    const struct d_ssl_config* _config,
    struct d_ssl_plan*         _out_plan
)
{
    struct d_ssl_plan plan     = { D_SSL_ROLE_CLIENT, D_SSL_VERSION_NONE,
                                   D_SSL_VERSION_NONE, false, false, false,
                                   false, false, false, false };
    enum d_ssl_error  status   = D_SSL_OK;
    bool              verifies = false;
    bool              server   = false;

    // parameter validation
    if ( (_config == NULL) ||
         (_out_plan == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    status = d_ssl_config_validate(_config);

    if (status != D_SSL_OK)
    {
        return status;
    }

    verifies = (_config->verify != D_SSL_VERIFY_NONE);
    server   = (_config->role == D_SSL_ROLE_SERVER);

    plan.role                = _config->role;
    plan.min_version         = _config->min_version;
    plan.max_version         = (_config->max_version == D_SSL_VERSION_NONE)
                                   ? D_SSL_VERSION_NEWEST
                                   : _config->max_version;
    plan.verify_chain        = ( (verifies) &&
                                 (!_config->pin_only) );
    plan.request_certificate = ( (server) &&
                                 (verifies) );
    plan.require_certificate = (_config->verify == D_SSL_VERIFY_REQUIRED);
    plan.verify_hostname     = ( (!server)   &&
                                 (verifies)  &&
                                 (_config->verify_hostname) );
    plan.check_pins          = (_config->pin_count > 0);
    plan.offer_alpn          = (_config->alpn.size > 0);
    plan.require_alpn        = _config->alpn_required;

    *_out_plan = plan;

    return D_SSL_OK;
}

//==============================================================================
// 13.  CONTEXTS
//==============================================================================

/*
d_internal_ssl_engine_status
  Holds an engine to its contract. OK, the known formal statuses,
OUT_OF_MEMORY, and BACKEND pass as they are; so do WANT_READ and
CLOSED where the operation may return them. Anything else --
WOULD_BLOCK and TRANSPORT above all, which only a transport may report
-- becomes BACKEND, so a misbehaving engine can neither make the kernel
retry forever nor pass itself off as the network.
*/
static enum d_ssl_error
d_internal_ssl_engine_status(
    enum d_ssl_error _status,
    bool             _may_want,
    bool             _may_close
)
{
    if ( (_status == D_SSL_OK)                               ||
         ( (d_ssl_error_is_formal(_status)) &&
           (d_ssl_error_name(_status) != NULL) )             ||
         (_status == D_SSL_ERROR_OUT_OF_MEMORY)              ||
         (_status == D_SSL_ERROR_BACKEND)                    ||
         ( (_may_want) &&
           (_status == D_SSL_ERROR_WANT_READ) )              ||
         ( (_may_close) &&
           (_status == D_SSL_ERROR_CLOSED) ) )
    {
        return _status;
    }

    return D_SSL_ERROR_BACKEND;
}

/*
d_internal_ssl_engine_complete
  Every operation the kernel calls unconditionally is present. peer_name and
peer_certificate are optional; d_internal_ssl_fit_engine decides whether a
given plan can do without them.
*/
static bool
d_internal_ssl_engine_complete(
    const struct d_ssl_engine* _engine
)
{
    return ( (_engine->context_create != NULL)   &&
             (_engine->context_destroy != NULL)  &&
             (_engine->session_create != NULL)   &&
             (_engine->session_destroy != NULL)  &&
             (_engine->feed != NULL)             &&
             (_engine->drain != NULL)            &&
             (_engine->handshake != NULL)        &&
             (_engine->read != NULL)             &&
             (_engine->write != NULL)            &&
             (_engine->close != NULL)            &&
             (_engine->describe != NULL) );
}

/*
d_internal_ssl_fit_engine
  Reconciles a plan with what the engine declares it can do. A maximum version
is a ceiling, so it drops to the engine's newest; a minimum is a requirement,
so it does not. Everything else the configuration needs and the engine lacks
is refused here, before a handshake could quietly do less than was asked.
*/
static enum d_ssl_error
d_internal_ssl_fit_engine(
    const struct d_ssl_config* _config,
    const struct d_ssl_engine* _engine,
    struct d_ssl_plan*         _plan
)
{
    uint32_t features     = _engine->features;
    bool     client_auth  = false;
    bool     other_anchor = false;
    bool     memory_pem   = false;

    // the version window
    if ( ((features & D_SSL_ENGINE_TLS1_3) == 0) &&
         ((int)_plan->max_version > (int)D_SSL_VERSION_TLS1_2) )
    {
        _plan->max_version = D_SSL_VERSION_TLS1_2;
    }

    if ( ( ((features & D_SSL_ENGINE_LEGACY) == 0) &&
           ((int)_plan->min_version < (int)D_SSL_VERSION_TLS1_2) ) ||
         ((int)_plan->min_version > (int)_plan->max_version) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    // trust anchors: the platform store alone needs an engine that can read it
    other_anchor = ( (_config->ca_file != NULL) ||
                     (_config->ca_path != NULL) ||
                     (_config->ca_pem.length > 0) );
    memory_pem   = ( (_config->ca_pem.length > 0)          ||
                     (_config->certificate_pem.length > 0) ||
                     (_config->private_key_pem.length > 0) );

    if ( ( (_plan->verify_chain)                             &&
           (_config->use_system_trust)                       &&
           (!other_anchor)                                   &&
           ((features & D_SSL_ENGINE_SYSTEM_TRUST) == 0) )   ||
         ( (memory_pem) &&
           ((features & D_SSL_ENGINE_MEMORY_PEM) == 0) ) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    // client certificates, presented or requested
    client_auth = ( (_plan->request_certificate) ||
                    ( (_plan->role == D_SSL_ROLE_CLIENT) &&
                      ( (_config->certificate_file != NULL) ||
                        (_config->certificate_pem.length > 0) ) ) );

    if ( ( (client_auth) &&
           ((features & D_SSL_ENGINE_CLIENT_AUTH) == 0) )   ||
         ( (_plan->offer_alpn) &&
           ((features & D_SSL_ENGINE_ALPN) == 0) )          ||
         ( (_config->keylog != NULL) &&
           ((features & D_SSL_ENGINE_KEYLOG) == 0) ) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    // names and pins are the kernel's to check, with the engine's help
    if ( ( (_plan->verify_hostname)                         &&
           (_engine->peer_name == NULL)                     &&
           ((features & D_SSL_ENGINE_HOSTNAME) == 0) )      ||
         ( (_plan->check_pins) &&
           (_engine->peer_certificate == NULL) ) )
    {
        return D_SSL_ERROR_UNSUPPORTED;
    }

    return D_SSL_OK;
}

/*
d_ssl_context_init
  Plan, fit, create. The context is filled only once the engine's context
exists, so every failure leaves it empty.
*/
enum d_ssl_error
d_ssl_context_init(
    struct d_ssl_context*      _context,
    const struct d_ssl_engine* _engine,
    const struct d_ssl_config* _config
)
{
    struct d_ssl_plan plan   = { D_SSL_ROLE_CLIENT, D_SSL_VERSION_NONE,
                                 D_SSL_VERSION_NONE, false, false, false,
                                 false, false, false, false };
    void*             handle = NULL;
    enum d_ssl_error  status = D_SSL_OK;

    // parameter validation
    if (_context == NULL)
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    memset(_context,
           0,
           sizeof(*_context));

    if ( (_engine == NULL) ||
         (_config == NULL) ||
         (!d_internal_ssl_engine_complete(_engine)) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    status = d_ssl_config_plan(_config,
                               &plan);

    if (status == D_SSL_OK)
    {
        status = d_internal_ssl_fit_engine(_config,
                                           _engine,
                                           &plan);
    }

    if (status == D_SSL_OK)
    {
        status = d_internal_ssl_engine_status(
                     _engine->context_create(_config,
                                             &plan,
                                             &handle),
                     false,
                     false);
    }

    if (status != D_SSL_OK)
    {
        return status;
    }

    _context->engine    = _engine;
    _context->handle    = handle;
    _context->plan      = plan;
    _context->pin_count = _config->pin_count;
    memcpy(_context->pins,
           _config->pins,
           sizeof(_context->pins));

    return D_SSL_OK;
}

/*
d_ssl_context_destroy
  An empty context has no engine, which is what makes a second destroy
harmless.
*/
void
d_ssl_context_destroy(
    struct d_ssl_context* _context
)
{
    if (_context == NULL)
    {
        return;
    }

    if (_context->engine != NULL)
    {
        _context->engine->context_destroy(_context->handle);
    }

    memset(_context,
           0,
           sizeof(*_context));

    return;
}

//==============================================================================
// 14.  SESSIONS
//==============================================================================

// the table of a session's connection view, which section 15 defines
static const struct d_net_connection_vtable* d_internal_ssl_view_table(void);

/*
d_internal_ssl_session_ok
  A session is usable from a successful d_ssl_session_init until
d_ssl_session_destroy clears it.
*/
static bool
d_internal_ssl_session_ok(
    const struct d_ssl_session* _session
)
{
    return ( (_session != NULL)          &&
             (_session->context != NULL) &&
             (_session->context->engine != NULL) );
}

/*
d_internal_ssl_session_clear
  Empties a session record, stage included, and points its base back at the
view's table, so a session cleared by a failed init or by destroy is still
a connection -- a closed one.
*/
static void
d_internal_ssl_session_clear(
    struct d_ssl_session* _session
)
{
    memset(_session,
           0,
           sizeof(*_session));
    (void)d_net_connection_init(&_session->base,
                                d_internal_ssl_view_table());

    return;
}

/*
d_internal_ssl_info_reset
  What is known before anything is negotiated: no version, no alerts, and a
peer not yet authenticated.
*/
static void
d_internal_ssl_info_reset(
    struct d_ssl_info* _info
)
{
    memset(_info,
           0,
           sizeof(*_info));

    _info->version        = D_SSL_VERSION_NONE;
    _info->verify         = D_SSL_VERIFY_FLAG_NOT_PERFORMED;
    _info->alert_sent     = D_SSL_ALERT_NONE;
    _info->alert_received = D_SSL_ALERT_NONE;

    return;
}

/*
d_internal_ssl_capture
  Refreshes the session's info from the engine. Engine text is re-terminated,
since the kernel hands it on as C strings. What the kernel owns survives:
the fingerprint it computed, and, once its checks have run, the verification
result they settled.
*/
static void
d_internal_ssl_capture(
    struct d_ssl_session* _session
)
{
    struct d_ssl_info fresh = _session->info;

    d_internal_ssl_info_reset(&fresh);
    _session->context->engine->describe(_session->handle,
                                        &fresh);

    fresh.cipher[D_SSL_CIPHER_NAME_MAX - 1] = '\0';

    if (fresh.alpn_length > D_SSL_ALPN_PROTOCOL_MAX)
    {
        fresh.alpn_length = 0;
    }

    fresh.alpn[fresh.alpn_length] = '\0';
    fresh.has_peer_certificate    = _session->info.has_peer_certificate;
    memcpy(fresh.fingerprint,
           _session->info.fingerprint,
           sizeof(fresh.fingerprint));

    if (_session->checked)
    {
        fresh.verify = _session->info.verify;
    }

    _session->info = fresh;

    return;
}

/*
d_internal_ssl_fail
  Ends a session. The failure is recorded, and every later call repeats it.
*/
static enum d_ssl_error
d_internal_ssl_fail(
    struct d_ssl_session* _session,
    enum d_ssl_error      _status
)
{
    _session->state   = D_SSL_STATE_FAILED;
    _session->failure = _status;
    _session->want    = D_SSL_WANT_NOTHING;

    return _status;
}

/*
d_internal_ssl_lost
  What a transport's failed write means to a session: a peer that has gone
-- the stream closed, reset, or aborted -- no longer reads, which is CLOSED;
anything else is a broken transport.
*/
static enum d_ssl_error
d_internal_ssl_lost(
    enum d_net_error _error
)
{
    return ( (_error == D_NET_ERROR_CLOSED)             ||
             (_error == D_NET_ERROR_CONNECTION_RESET)   ||
             (_error == D_NET_ERROR_CONNECTION_ABORTED) )
           ? D_SSL_ERROR_CLOSED
           : D_SSL_ERROR_TRANSPORT;
}

/*
d_internal_ssl_flush
  Moves ciphertext from the engine to the transport through the stage, until
the engine has nothing left or the transport stops taking bytes. The stage is
refilled only once empty, so bytes leave in the order the engine produced
them however the transport splits its writes; what a write took counts
before what it reported. Returns the transport's WOULD_BLOCK (noting that
the session waits to write) or a departed peer's CLOSED, for the caller to
judge, and any other trouble as TRANSPORT or BACKEND.
*/
static enum d_ssl_error
d_internal_ssl_flush(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_error           status = D_SSL_OK;

    for (;;)
    {
        size_t pending = _session->stage_end - _session->stage_start;
        size_t moved   = 0;

        // an empty stage is refilled from the engine, if it has anything
        if (pending == 0)
        {
            _session->stage_start = 0;
            _session->stage_end   = 0;

            status = engine->drain(_session->handle,
                                   _session->stage,
                                   sizeof(_session->stage),
                                   &moved);

            if ( (status != D_SSL_OK) ||
                 (moved > sizeof(_session->stage)) )
            {
                return D_SSL_ERROR_BACKEND;
            }

            if (moved == 0)
            {
                return D_SSL_OK;
            }

            _session->stage_end = moved;
            pending             = moved;
            moved               = 0;
        }

        const struct d_net_io_result result =
            d_net_connection_write(_session->transport,
                                   _session->stage + _session->stage_start,
                                   pending);

        _session->transport_error  = result.error;
        _session->stage_start     += result.count;
        _session->bytes_out       += result.count;

        // the transport will take more later
        if (result.error == D_NET_ERROR_WOULD_BLOCK)
        {
            _session->want = D_SSL_WANT_WRITE;

            return D_SSL_ERROR_WOULD_BLOCK;
        }

        // a failed write, or one that took nothing and said nothing
        if (result.error != D_NET_OK)
        {
            return d_internal_ssl_lost(result.error);
        }

        if (result.count == 0)
        {
            return D_SSL_ERROR_TRANSPORT;
        }
    }
}

/*
d_internal_ssl_sniff_input
  Collects the peer's first bytes, up to a record header's worth, and judges
them. Once they are judged TLS the session stops looking; NOT_TLS ends it
before the engine has parsed a byte of whatever the peer is speaking.
*/
static enum d_ssl_error
d_internal_ssl_sniff_input(
    struct d_ssl_session* _session,
    const unsigned char*  _data,
    size_t                _size
)
{
    size_t take = D_SSL_RECORD_HEADER_SIZE - _session->sniff_length;

    if (take > _size)
    {
        take = _size;
    }

    memcpy(_session->sniff + _session->sniff_length,
           _data,
           take);
    _session->sniff_length += take;

    switch (d_ssl_sniff(d_internal_ssl_bytes(_session->sniff,
                                             _session->sniff_length),
                        NULL))
    {
        case D_SSL_SNIFF_NOT_TLS:
            return D_SSL_ERROR_NOT_TLS;

        case D_SSL_SNIFF_TLS:
        case D_SSL_SNIFF_SSL2:
            _session->sniffed = true;
            break;

        case D_SSL_SNIFF_NEED_MORE:
            break;
    }

    return D_SSL_OK;
}

/*
d_internal_ssl_pull
  One transport read, sniffed if the session is still judging its peer, then
fed to the engine whole. Bytes that arrived are taken whatever the read
reported with them: a transport that failed after them says so again on the
next read. With no bytes, returns the transport's WOULD_BLOCK (noting that
the session waits to read), the end of its stream as CLOSED, for the caller
to judge, or a failure as TRANSPORT.
*/
static enum d_ssl_error
d_internal_ssl_pull(
    struct d_ssl_session* _session
)
{
    unsigned char    chunk[D_SSL_IO_CAPACITY] = { 0 };
    enum d_ssl_error status                   = D_SSL_OK;

    const struct d_net_io_result result =
        d_net_connection_read(_session->transport,
                              chunk,
                              sizeof(chunk));
    const size_t                 count  = result.count;

    _session->transport_error = result.error;

    // nothing arrived: a stall, the end of the stream, or a failure
    if (count == 0)
    {
        // the transport will have more later
        if (result.error == D_NET_ERROR_WOULD_BLOCK)
        {
            _session->want = D_SSL_WANT_READ;

            return D_SSL_ERROR_WOULD_BLOCK;
        }

        return (result.error == D_NET_OK) ? D_SSL_ERROR_CLOSED
                                          : D_SSL_ERROR_TRANSPORT;
    }

    _session->bytes_in += count;

    if (!_session->sniffed)
    {
        status = d_internal_ssl_sniff_input(_session,
                                            chunk,
                                            count);

        if (status != D_SSL_OK)
        {
            return status;
        }
    }

    return d_internal_ssl_engine_status(
               _session->context->engine->feed(_session->handle,
                                               chunk,
                                               count),
               false,
               false);
}

/*
d_internal_ssl_await
  An engine step answered WANT_READ. Whatever the step produced goes out first
-- the peer may be waiting on exactly that -- and only then is the transport
read. OK means the step may be retried.
*/
static enum d_ssl_error
d_internal_ssl_await(
    struct d_ssl_session* _session
)
{
    enum d_ssl_error status = d_internal_ssl_flush(_session);

    if (status != D_SSL_OK)
    {
        return status;
    }

    return d_internal_ssl_pull(_session);
}

/*
d_internal_ssl_io_failure
  Judges what flush, pull, or await returned. WOULD_BLOCK is not a failure:
the caller retries later. A transport that ends, in either direction, before
the TLS stream has is TRUNCATED -- never CLOSED, which is
reserved for close_notify, so truncation cannot pass for a clean end.
Everything else fails the session as it is.
*/
static enum d_ssl_error
d_internal_ssl_io_failure(
    struct d_ssl_session* _session,
    enum d_ssl_error      _status
)
{
    if (_status == D_SSL_ERROR_WOULD_BLOCK)
    {
        return _status;
    }

    if (_status == D_SSL_ERROR_CLOSED)
    {
        return d_internal_ssl_fail(_session,
                                   D_SSL_ERROR_TRUNCATED);
    }

    return d_internal_ssl_fail(_session,
                               _status);
}

/*
d_internal_ssl_fail_engine
  An engine step failed. Its info is captured for the caller, the alert it
queued is sent if the transport takes it at once, and the session fails.
*/
static enum d_ssl_error
d_internal_ssl_fail_engine(
    struct d_ssl_session* _session,
    enum d_ssl_error      _status
)
{
    d_internal_ssl_capture(_session);

    // best effort: a blocked or broken transport just loses the alert
    (void)d_internal_ssl_flush(_session);

    return d_internal_ssl_fail(_session,
                               _status);
}

/*
d_internal_ssl_names_match
  Asks the engine for the peer certificate's names one at a time, matching
each against the session's host. The walk is bounded, so an engine that never
says "no more" cannot hang a handshake.
*/
static bool
d_internal_ssl_names_match(
    const struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    struct d_pack_text         host   = { _session->host,
                                          strlen(_session->host) };
    struct d_ssl_peer_name     name   = { D_SSL_NAME_DNS, { NULL, 0 } };

    for (size_t i = 0; i < D_INTERNAL_SSL_PEER_NAMES_MAX; ++i)
    {
        if (!engine->peer_name(_session->handle,
                               i,
                               &name))
        {
            break;
        }

        if (d_internal_ssl_peer_name_matches(&name,
                                             host,
                                             _session->host_is_ip))
        {
            return true;
        }
    }

    return false;
}

/*
d_internal_ssl_pins_match
  Compares against every pin, match or not, so timing reveals nothing about
which pin matched or how closely the others came.
*/
static bool
d_internal_ssl_pins_match(
    const struct d_ssl_context* _context,
    const uint8_t               _fingerprint[D_SSL_FINGERPRINT_LENGTH]
)
{
    bool matched = false;

    for (size_t i = 0; i < _context->pin_count; ++i)
    {
        matched = ( (d_ssl_fingerprint_equal(_context->pins[i],
                                             _fingerprint)) ||
                    (matched) );
    }

    return matched;
}

/*
d_internal_ssl_peer_present
  Whether the peer presented a certificate. An engine that can hand the
certificate over has it fingerprinted here, whichever engine it is; one that
cannot has said so in its verification flags.
*/
static bool
d_internal_ssl_peer_present(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine  = _session->context->engine;
    struct d_ssl_info*         info    = &_session->info;
    struct d_pack_bytes        der     = { NULL, 0 };
    bool                       present = false;

    if (engine->peer_certificate == NULL)
    {
        return ((info->verify & D_SSL_VERIFY_FLAG_NO_CERTIFICATE) == 0);
    }

    present = ( (engine->peer_certificate(_session->handle,
                                          &der)) &&
                (der.data != NULL)               &&
                (der.size > 0) );

    if ( (present) &&
         (d_ssl_fingerprint_compute(der,
                                    info->fingerprint) ==
          D_SSL_OK) )
    {
        info->has_peer_certificate = true;
    }

    return present;
}

/*
d_internal_ssl_peer_verdict
  The verification flags for a peer that presented a certificate: the
engine's chain verdict, unless pins alone establish trust; a pin mismatch;
and a name mismatch, checked here for an engine that reports the names.
*/
static uint32_t
d_internal_ssl_peer_verdict(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_context* context = _session->context;
    const struct d_ssl_plan*    plan    = &context->plan;
    const struct d_ssl_engine*  engine  = context->engine;
    uint32_t                    verify  = D_SSL_VERIFY_FLAG_NONE;

    // the engine's chain verdict, unless pins alone establish trust
    if (plan->verify_chain)
    {
        verify |= _session->info.verify;
    }

    if ( (plan->check_pins) &&
         (!d_internal_ssl_pins_match(context,
                                     _session->info.fingerprint)) )
    {
        verify |= D_SSL_VERIFY_FLAG_PIN_MISMATCH;
    }

    // an engine without peer_name checked the name itself, as context
    // creation required; one with it is checked here as well
    if ( (plan->verify_hostname)            &&
         (engine->peer_name != NULL)        &&
         (!d_internal_ssl_names_match(_session)) )
    {
        verify |= D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH;
    }

    return verify;
}

/*
d_internal_ssl_check_peer
  The kernel's checks on a finished engine handshake, run while its last
flight is still inside the engine. The engine enforced the plan within the
handshake; these confirm the outcome, and add the checks the kernel makes for
every engine alike. The verification result is settled here, and a failure
is VERIFY with the flags saying why.
*/
static enum d_ssl_error
d_internal_ssl_check_peer(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_plan* plan    = &_session->context->plan;
    struct d_ssl_info*       info    = &_session->info;
    bool                     present = false;

    d_internal_ssl_capture(_session);

    // the version window, which a correct engine never breaches
    if ( (!d_ssl_version_is_known(info->version))              ||
         ((int)info->version < (int)plan->min_version)         ||
         ((int)info->version > (int)plan->max_version) )
    {
        return D_SSL_ERROR_HANDSHAKE;
    }

    if ( (plan->require_alpn) &&
         (info->alpn_length == 0) )
    {
        return D_SSL_ERROR_HANDSHAKE;
    }

    // the peer certificate, fingerprinted here whichever engine produced it
    present = d_internal_ssl_peer_present(_session);

    // a peer no one asked to authenticate is recorded as such
    if ( (!plan->verify_chain) &&
         (!plan->check_pins) )
    {
        info->verify = D_SSL_VERIFY_FLAG_NOT_PERFORMED;

        return D_SSL_OK;
    }

    // without a certificate: fatal if required, else an unauthenticated peer
    if (!present)
    {
        info->verify = D_SSL_VERIFY_FLAG_NO_CERTIFICATE;

        if (plan->require_certificate)
        {
            return D_SSL_ERROR_VERIFY;
        }

        info->verify |= D_SSL_VERIFY_FLAG_NOT_PERFORMED;

        return D_SSL_OK;
    }

    const uint32_t verify = d_internal_ssl_peer_verdict(_session);

    info->verify = verify;

    return (verify == D_SSL_VERIFY_FLAG_NONE) ? D_SSL_OK
                                              : D_SSL_ERROR_VERIFY;
}

/*
d_internal_ssl_handshake_rounds
  The engine's part of a handshake. Each round flushes what the engine queued,
steps it, and, when it waits on the peer, flushes again and reads. Returns OK
once the engine reports completion, which engine_done records; otherwise the
transport's WOULD_BLOCK, for the caller to resume, or the failure that ended
the session.
*/
static enum d_ssl_error
d_internal_ssl_handshake_rounds(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_error           status = D_SSL_OK;

    while (!_session->engine_done)
    {
        // what the engine queued goes out before it is asked for more
        status = d_internal_ssl_flush(_session);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_io_failure(_session,
                                             status);
        }

        status = d_internal_ssl_engine_status(
                     engine->handshake(_session->handle),
                     true,
                     true);

        // a close_notify before the handshake finished is a refusal
        if (status == D_SSL_ERROR_CLOSED)
        {
            status = D_SSL_ERROR_HANDSHAKE;
        }

        if (status == D_SSL_OK)
        {
            _session->engine_done = true;

            break;
        }

        if (status != D_SSL_ERROR_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session,
                                              status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_io_failure(_session,
                                             status);
        }
    }

    return D_SSL_OK;
}

/*
d_internal_ssl_handshake
  Drives a handshake: the engine's rounds, then the kernel's checks, which run
while the engine's last flight is still inside it, so a peer that fails them
never receives it. The session is ESTABLISHED only once that flight is on the
wire; engine_done and checked let a WOULD_BLOCK anywhere resume at the right
step.
*/
static enum d_ssl_error
d_internal_ssl_handshake(
    struct d_ssl_session* _session
)
{
    enum d_ssl_error status = D_SSL_OK;

    _session->state = D_SSL_STATE_HANDSHAKING;

    // the engine's rounds, until it reports completion
    status = d_internal_ssl_handshake_rounds(_session);

    if (status != D_SSL_OK)
    {
        return status;
    }

    // the checks see the finished handshake before its last flight leaves
    if (!_session->checked)
    {
        status = d_internal_ssl_check_peer(_session);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_fail(_session,
                                       status);
        }

        _session->checked = true;
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_OK)
    {
        return d_internal_ssl_io_failure(_session,
                                         status);
    }

    _session->state = D_SSL_STATE_ESTABLISHED;
    _session->want  = D_SSL_WANT_NOTHING;

    return D_SSL_OK;
}

/*
d_internal_ssl_read
  The read pump of an established session. A WANT_READ may follow records
the engine absorbed without yielding data -- session tickets, key updates --
so the loop runs until data, the peer's close_notify, or a failure.
*/
static enum d_ssl_error
d_internal_ssl_read(
    struct d_ssl_session* _session,
    void*                 _buffer,
    size_t                _capacity,
    size_t*               _out_read
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_error           status = D_SSL_OK;

    for (;;)
    {
        size_t count = 0;

        status = d_internal_ssl_engine_status(
                     engine->read(_session->handle,
                                  _buffer,
                                  _capacity,
                                  &count),
                     true,
                     true);

        if (status == D_SSL_OK)
        {
            if ( (count == 0) ||
                 (count > _capacity) )
            {
                return d_internal_ssl_fail(_session,
                                           D_SSL_ERROR_BACKEND);
            }

            *_out_read = count;

            // replies the read provoked go out now if the transport allows;
            // a failure here resurfaces on the next call, not this one
            status         = d_internal_ssl_flush(_session);
            _session->want = (status == D_SSL_ERROR_WOULD_BLOCK)
                                 ? D_SSL_WANT_WRITE
                                 : D_SSL_WANT_NOTHING;

            return D_SSL_OK;
        }

        if (status == D_SSL_ERROR_CLOSED)
        {
            d_internal_ssl_capture(_session);

            _session->received_close = true;
            _session->want           = D_SSL_WANT_NOTHING;

            if (_session->sent_close)
            {
                _session->state = D_SSL_STATE_CLOSED;
            }

            return status;
        }

        if (status != D_SSL_ERROR_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session,
                                              status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_io_failure(_session,
                                             status);
        }
    }
}

/*
d_internal_ssl_write
  The write pump of an established session with nothing pending. Once the
engine has taken data it is committed; a transport that then blocks delays
its ciphertext but does not undo it.
*/
static enum d_ssl_error
d_internal_ssl_write(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_error           status = D_SSL_OK;

    for (;;)
    {
        size_t taken = 0;

        status = d_internal_ssl_engine_status(
                     engine->write(_session->handle,
                                   _data,
                                   _size,
                                   &taken),
                     true,
                     false);

        if (status == D_SSL_OK)
        {
            if ( (taken == 0) ||
                 (taken > _size) )
            {
                return d_internal_ssl_fail(_session,
                                           D_SSL_ERROR_BACKEND);
            }

            *_out_written = taken;
            status        = d_internal_ssl_flush(_session);

            // committed data waits in the session; wants() says WRITE
            if (status == D_SSL_ERROR_WOULD_BLOCK)
            {
                return D_SSL_OK;
            }

            if (status != D_SSL_OK)
            {
                return d_internal_ssl_io_failure(_session,
                                                 status);
            }

            _session->want = D_SSL_WANT_NOTHING;

            return D_SSL_OK;
        }

        if (status != D_SSL_ERROR_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session,
                                              status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_io_failure(_session,
                                             status);
        }
    }
}

/*
d_internal_ssl_session_host
  Validates the host for a session and copies it in without its trailing dot.
A server names no host, and a client that checks names must name one; a host
is an address literal or a valid name.
*/
static enum d_ssl_error
d_internal_ssl_session_host(
    struct d_ssl_session*       _session,
    const struct d_ssl_context* _context,
    struct d_pack_text          _host
)
{
    const struct d_pack_text host = d_internal_ssl_strip_dot(_host);

    // a server names no host; a client that checks names must name one
    if ( ( (_context->plan.role == D_SSL_ROLE_SERVER) &&
           (_host.length > 0) )                         ||
         ( (_context->plan.verify_hostname) &&
           (host.length == 0) ) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    // a host is an address literal or a valid name
    if (_host.length > 0)
    {
        _session->host_is_ip = d_ssl_host_is_ip(host);

        if ( (!_session->host_is_ip) &&
             (!d_ssl_hostname_is_valid(_host)) )
        {
            _session->host_is_ip = false;

            return D_SSL_ERROR_MALFORMED;
        }

        memcpy(_session->host,
               host.data,
               host.length);
        _session->host[host.length] = '\0';
    }

    return D_SSL_OK;
}

/*
d_ssl_session_init
  The record is cleared first, so every early return leaves it empty, its
base a closed connection. The host is validated and copied without its
trailing dot; the engine receives it as the reference identity only when
the plan checks names, and as SNI only when it is a name rather than an
address.
*/
enum d_ssl_error
d_ssl_session_init(
    struct d_ssl_session*       _session,
    const struct d_ssl_context* _context,
    struct d_net_connection*    _transport,
    struct d_pack_text          _host
)
{
    const char*      name   = NULL;
    const char*      sni    = NULL;
    void*            handle = NULL;
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if (_session == NULL)
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    d_internal_ssl_session_clear(_session);

    if ( (_context == NULL)                          ||
         (_context->engine == NULL)                  ||
         (!d_net_connection_is_complete(_transport)) ||
         (!d_internal_ssl_text_ok(_host)) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    status = d_internal_ssl_session_host(_session,
                                         _context,
                                         _host);

    if (status != D_SSL_OK)
    {
        return status;
    }

    name = (_context->plan.verify_hostname) ? _session->host : NULL;
    sni  = ( (_session->host[0] != '\0') &&
             (!_session->host_is_ip) ) ? _session->host : NULL;

    status = d_internal_ssl_engine_status(
                 _context->engine->session_create(_context->handle,
                                                  name,
                                                  sni,
                                                  &handle),
                 false,
                 false);

    if (status != D_SSL_OK)
    {
        d_internal_ssl_session_clear(_session);

        return status;
    }

    _session->context         = _context;
    _session->handle          = handle;
    _session->transport       = _transport;
    _session->transport_error = D_NET_OK;
    _session->state           = D_SSL_STATE_IDLE;
    _session->failure         = D_SSL_OK;
    _session->want            = D_SSL_WANT_NOTHING;
    _session->owns_transport  = false;
    _session->view_closed     = false;
    d_internal_ssl_info_reset(&_session->info);

    return D_SSL_OK;
}

/*
d_ssl_session_resume
  Every check is the kernel's, so an engine's hook sees only a client
session that has not begun and an established one of its own context; the
offer itself is the engine's, and a refusal by the server is not a failure
here -- the handshake reports it.
*/
enum d_ssl_error
d_ssl_session_resume(
    struct d_ssl_session*       _session,
    const struct d_ssl_session* _from
)
{
    // parameter validation
    if ( (_session == NULL)                                  ||
         (_from == NULL)                                     ||
         (_session->context == NULL)                         ||
         (_session->context != _from->context)               ||
         (_session->context->plan.role == D_SSL_ROLE_SERVER) ||
         (_session->state != D_SSL_STATE_IDLE)               ||
         (_from->state != D_SSL_STATE_ESTABLISHED) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    // an engine without the hook offers nothing
    if (_session->context->engine->session_resume == NULL)
    {
        return D_SSL_OK;
    }

    return _session->context->engine->session_resume(_session->handle,
                                                     _from->handle);
}

/*
d_ssl_session_destroy
  Hands the engine's session back and clears the record, stage included; the
transport is the caller's, and destroying is not closing.
*/
void
d_ssl_session_destroy(
    struct d_ssl_session* _session
)
{
    if (_session == NULL)
    {
        return;
    }

    if (d_internal_ssl_session_ok(_session))
    {
        _session->context->engine->session_destroy(_session->handle);
    }

    d_internal_ssl_session_clear(_session);

    return;
}

/*
d_ssl_session_handshake
  Dispatches on the state; the work is d_internal_ssl_handshake's.
*/
enum d_ssl_error
d_ssl_session_handshake(
    struct d_ssl_session* _session
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    switch (_session->state)
    {
        case D_SSL_STATE_FAILED:
            return _session->failure;

        case D_SSL_STATE_CLOSED:
            return D_SSL_ERROR_STATE;

        case D_SSL_STATE_ESTABLISHED:
            status = d_internal_ssl_flush(_session);

            if (status != D_SSL_OK)
            {
                return d_internal_ssl_io_failure(_session,
                                                 status);
            }

            _session->want = D_SSL_WANT_NOTHING;

            return D_SSL_OK;

        case D_SSL_STATE_IDLE:
        case D_SSL_STATE_HANDSHAKING:
            break;
    }

    return d_internal_ssl_handshake(_session);
}

/*
d_ssl_session_read
  The peer's close_notify is checked before the state, so it keeps reading as
the end of the stream even once this end has closed too.
*/
enum d_ssl_error
d_ssl_session_read(
    struct d_ssl_session* _session,
    void*                 _buffer,
    size_t                _capacity,
    size_t*               _out_read
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         (_buffer == NULL)                      ||
         (_capacity == 0)                       ||
         (_out_read == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    *_out_read = 0;

    if (_session->state == D_SSL_STATE_FAILED)
    {
        return _session->failure;
    }

    if (_session->received_close)
    {
        return D_SSL_ERROR_CLOSED;
    }

    if (_session->state == D_SSL_STATE_CLOSED)
    {
        return D_SSL_ERROR_STATE;
    }

    // a read handshakes first, as a transport's user expects
    if (_session->state != D_SSL_STATE_ESTABLISHED)
    {
        status = d_internal_ssl_handshake(_session);

        if (status != D_SSL_OK)
        {
            return status;
        }
    }

    return d_internal_ssl_read(_session,
                               _buffer,
                               _capacity,
                               _out_read);
}

/*
d_ssl_session_write
  Earlier ciphertext goes first, and new data is taken only once it has gone:
a session never holds more than one write's worth unsent.
*/
enum d_ssl_error
d_ssl_session_write(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         (_data == NULL)                        ||
         (_size == 0)                           ||
         (_out_written == NULL) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    *_out_written = 0;

    if (_session->state == D_SSL_STATE_FAILED)
    {
        return _session->failure;
    }

    // nothing may follow this end's close_notify
    if ( (_session->state == D_SSL_STATE_CLOSED) ||
         (_session->sent_close) )
    {
        return D_SSL_ERROR_STATE;
    }

    if (_session->state != D_SSL_STATE_ESTABLISHED)
    {
        status = d_internal_ssl_handshake(_session);

        if (status != D_SSL_OK)
        {
            return status;
        }
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_OK)
    {
        return d_internal_ssl_io_failure(_session,
                                         status);
    }

    return d_internal_ssl_write(_session,
                                _data,
                                _size,
                                _out_written);
}

/*
d_ssl_session_write_all
  Stops at the first status other than OK, reporting how far it got; a write
that blocked after taking data returns OK with the rest pending, so the loop
meets the blockage on its next write and reports it there.
*/
enum d_ssl_error
d_ssl_session_write_all(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    const unsigned char* data   = (const unsigned char*)_data;
    size_t               done   = 0;
    enum d_ssl_error     status = D_SSL_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         ( (_data == NULL) &&
           (_size > 0) ) )
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    while (done < _size)
    {
        size_t taken = 0;

        status = d_ssl_session_write(_session,
                                     data + done,
                                     _size - done,
                                     &taken);
        done  += taken;

        if (status != D_SSL_OK)
        {
            break;
        }
    }

    if (_out_written != NULL)
    {
        *_out_written = done;
    }

    if (status != D_SSL_OK)
    {
        return status;
    }

    return d_ssl_session_flush(_session);
}

/*
d_ssl_session_flush
  Valid in any state but FAILED; an idle session simply has nothing to send.
*/
enum d_ssl_error
d_ssl_session_flush(
    struct d_ssl_session* _session
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    if (_session->state == D_SSL_STATE_FAILED)
    {
        return _session->failure;
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_OK)
    {
        return d_internal_ssl_io_failure(_session,
                                         status);
    }

    _session->want = D_SSL_WANT_NOTHING;

    return D_SSL_OK;
}

/*
d_ssl_session_shutdown
  close_notify is queued once, behind anything already pending, and flushed.
A peer that closed first may already have gone; nothing further is owed to
it, so its departure is not an error here, and what could not be sent is
dropped.
*/
enum d_ssl_error
d_ssl_session_shutdown(
    struct d_ssl_session* _session
)
{
    enum d_ssl_error status = D_SSL_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_ERROR_INVALID_ARGUMENT;
    }

    switch (_session->state)
    {
        case D_SSL_STATE_FAILED:
            return _session->failure;

        case D_SSL_STATE_CLOSED:
            return D_SSL_OK;

        // nothing established means nothing to close: just stop
        case D_SSL_STATE_IDLE:
        case D_SSL_STATE_HANDSHAKING:
            _session->state = D_SSL_STATE_CLOSED;
            _session->want  = D_SSL_WANT_NOTHING;

            return D_SSL_OK;

        case D_SSL_STATE_ESTABLISHED:
            break;
    }

    if (!_session->sent_close)
    {
        status = d_internal_ssl_engine_status(
                     _session->context->engine->close(_session->handle),
                     false,
                     false);

        if (status != D_SSL_OK)
        {
            return d_internal_ssl_fail_engine(_session,
                                              status);
        }

        _session->sent_close = true;
    }

    status = d_internal_ssl_flush(_session);

    if ( (status == D_SSL_ERROR_CLOSED) &&
         (_session->received_close) )
    {
        _session->stage_start = 0;
        _session->stage_end   = 0;
        status                = D_SSL_OK;
    }

    if (status != D_SSL_OK)
    {
        return d_internal_ssl_io_failure(_session,
                                         status);
    }

    if (_session->received_close)
    {
        _session->state = D_SSL_STATE_CLOSED;
    }

    _session->want = D_SSL_WANT_NOTHING;

    return D_SSL_OK;
}

/*
d_ssl_session_state
  An unusable session reads as FAILED, which no caller mistakes for progress.
*/
enum d_ssl_state
d_ssl_session_state(
    const struct d_ssl_session* _session
)
{
    return d_internal_ssl_session_ok(_session) ? _session->state
                                               : D_SSL_STATE_FAILED;
}

/*
d_ssl_session_wants
  Set by the pump when the transport blocks; cleared when a call completes.
*/
enum d_ssl_want
d_ssl_session_wants(
    const struct d_ssl_session* _session
)
{
    return d_internal_ssl_session_ok(_session) ? _session->want
                                               : D_SSL_WANT_NOTHING;
}

/*
d_ssl_session_info
  A copy, so the caller's record stays stable while the session moves on.
*/
bool
d_ssl_session_info(
    const struct d_ssl_session* _session,
    struct d_ssl_info*          _out
)
{
    if ( (!d_internal_ssl_session_ok(_session)) ||
         (_out == NULL) )
    {
        return false;
    }

    *_out = _session->info;

    return true;
}

/*
d_ssl_session_own_transport
  Only the connection view's close reads the flag.
*/
void
d_ssl_session_own_transport(
    struct d_ssl_session* _session,
    bool                  _owns
)
{
    // NULL is ignored
    if (_session != NULL)
    {
        _session->owns_transport = _owns;
    }

    return;
}

/*
d_ssl_session_failure
  The sticky failure, which a FAILED session holds and every other reports
as none.
*/
enum d_ssl_error
d_ssl_session_failure(
    const struct d_ssl_session* _session
)
{
    return ( (d_internal_ssl_session_ok(_session)) &&
             (_session->state == D_SSL_STATE_FAILED) ) ? _session->failure
                                                       : D_SSL_OK;
}

/*
d_ssl_session_transport_error
  Set by the pump on every transport call, so it is the last call's outcome.
*/
enum d_net_error
d_ssl_session_transport_error(
    const struct d_ssl_session* _session
)
{
    return d_internal_ssl_session_ok(_session) ? _session->transport_error
                                               : D_NET_OK;
}

//==============================================================================
// 15.  SESSIONS AS CONNECTIONS
//==============================================================================

/*
d_internal_ssl_view_error
  A session's failure in net.h's terms. Behind TRANSPORT stands the
transport's own error, which the session kept; truncation is a stream that
ended too soon; a closed session reads and writes as a closed connection;
the rest map as d_ssl_error_to_net maps them.
*/
static enum d_net_error
d_internal_ssl_view_error(
    const struct d_ssl_session* _session,
    enum d_ssl_error            _status
)
{
    switch (_status)
    {
        case D_SSL_ERROR_TRANSPORT:
            return (_session->transport_error != D_NET_OK)
                       ? _session->transport_error
                       : d_ssl_error_to_net(_status);

        case D_SSL_ERROR_TRUNCATED:
        case D_SSL_ERROR_STATE:
            return D_NET_ERROR_CLOSED;

        default:
            return d_ssl_error_to_net(_status);
    }
}

/*
d_internal_ssl_view_read
  The peer's close_notify is the end of the stream, a count of 0 with no
error, and d_ssl_session_read repeats it on every read after.
*/
static struct d_net_io_result
d_internal_ssl_view_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_ssl_session* const session = (struct d_ssl_session*)_connection;
    size_t                      count   = 0;

    // a destroyed session, or a closed view, is a closed connection
    if ( (!d_internal_ssl_session_ok(session)) ||
         (session->view_closed) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    const enum d_ssl_error status = d_ssl_session_read(session,
                                                       _buffer,
                                                       _size,
                                                       &count);

    // data, or the orderly end
    if ( (status == D_SSL_OK) ||
         (status == D_SSL_ERROR_CLOSED) )
    {
        return d_net_io_result_make(count,
                                    D_NET_OK);
    }

    return d_net_io_result_make(0u,
                                d_internal_ssl_view_error(session,
                                                          status));
}

/*
d_internal_ssl_view_write
  One d_ssl_session_write: at most a record's worth, which net.h's
write_all loops over.
*/
static struct d_net_io_result
d_internal_ssl_view_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    struct d_ssl_session* const session = (struct d_ssl_session*)_connection;
    size_t                      count   = 0;

    // a destroyed session, or a closed view, is a closed connection
    if ( (!d_internal_ssl_session_ok(session)) ||
         (session->view_closed) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    const enum d_ssl_error status = d_ssl_session_write(session,
                                                        _data,
                                                        _size,
                                                        &count);

    return (status == D_SSL_OK)
               ? d_net_io_result_make(count,
                                      D_NET_OK)
               : d_net_io_result_make(0u,
                                      d_internal_ssl_view_error(session,
                                                                status));
}

/*
d_internal_ssl_view_is_open
  Open until it fails, ends both ways, or its view closes.
*/
static bool
d_internal_ssl_view_is_open(
    const struct d_net_connection* _connection
)
{
    const struct d_ssl_session* const session =
        (const struct d_ssl_session*)_connection;

    return ( (d_internal_ssl_session_ok(session))            &&
             (!session->view_closed)                         &&
             (session->state != D_SSL_STATE_FAILED)          &&
             (session->state != D_SSL_STATE_CLOSED) );
}

/*
d_internal_ssl_view_close
  close_notify once, on an established session, as far as the transport
takes it at once; then the view is closed, whatever the peer has said, and
the transport with it when the session owns that. Closing again does
nothing. The session's own state is left as it stands, for a caller that
asks it why.
*/
static void
d_internal_ssl_view_close(
    struct d_net_connection* _connection
)
{
    struct d_ssl_session* const session = (struct d_ssl_session*)_connection;

    // a destroyed session, or a closed view, has nothing more to close
    if ( (!d_internal_ssl_session_ok(session)) ||
         (session->view_closed) )
    {
        return;
    }

    // an orderly end, when there is one to give
    if (session->state == D_SSL_STATE_ESTABLISHED)
    {
        (void)d_ssl_session_shutdown(session);
    }

    session->view_closed = true;

    // an owned transport goes with it
    if (session->owns_transport)
    {
        d_net_connection_close(session->transport);
    }

    return;
}

/*
d_internal_ssl_view_shutdown
  WRITE and BOTH end the session's sending with close_notify; then the
transport shuts the same direction down. The mode arrives validated.
*/
static enum d_net_error
d_internal_ssl_view_shutdown(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    struct d_ssl_session* const session = (struct d_ssl_session*)_connection;

    // a destroyed session, or a closed view, has nothing to shut down
    if ( (!d_internal_ssl_session_ok(session)) ||
         (session->view_closed) )
    {
        return D_NET_ERROR_CLOSED;
    }

    // sending ends with close_notify
    if (_mode != D_NET_SHUTDOWN_READ)
    {
        const enum d_ssl_error status = d_ssl_session_shutdown(session);

        // a close_notify that could not go out is the answer
        if (status != D_SSL_OK)
        {
            return d_internal_ssl_view_error(session,
                                             status);
        }
    }

    return d_net_connection_shutdown(session->transport,
                                     _mode);
}

/*
d_internal_ssl_view_remote
  The transport's peer.
*/
static bool
d_internal_ssl_view_remote(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return d_net_connection_remote_endpoint(
               ((const struct d_ssl_session*)_connection)->transport,
               _out);
}

/*
d_internal_ssl_view_local
  The transport's own end.
*/
static bool
d_internal_ssl_view_local(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return d_net_connection_local_endpoint(
               ((const struct d_ssl_session*)_connection)->transport,
               _out);
}

/*
d_internal_ssl_view_table
  The table every session's base points at. No destroy, the storage being
the caller's, and no available: plaintext waiting inside an engine is not
counted through its interface.
*/
static const struct d_net_connection_vtable*
d_internal_ssl_view_table(void)
{
    static const struct d_net_connection_vtable SESSION_OPERATIONS =
    {
        d_internal_ssl_view_read,
        d_internal_ssl_view_write,
        d_internal_ssl_view_is_open,
        d_internal_ssl_view_close,
        d_internal_ssl_view_shutdown,
        d_internal_ssl_view_remote,
        d_internal_ssl_view_local,
        NULL,
        NULL
    };

    return &SESSION_OPERATIONS;
}
