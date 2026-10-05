/*******************************************************************************
* djinterp [net]                                                           ssl.c
*
* Implementation of the shared SSL/TLS kernel declared in ssl.h.
*   Pure computation over caller-owned memory -- names, ALPN lists, PEM,
* SHA-256, configuration -- and the session pump, which moves ciphertext
* between an engine and a d_ssl_transport. The only I/O is through the
* transports the caller passes in, and the only TLS through the engine it
* names.
*
*
* path:      /src/djinterp/net/ssl/ssl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssl/ssl.h"  // corresponding header
// std
#include <string.h>  // memchr, memcmp, memcpy, memmove, memset, strlen
// djinterp
#include "../../../../inc/djinterp/env/net/ssl/env_ssl.h"  // D_ENV_SSL_CA_FILES


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
d_internal_ssl_lower
  ASCII-only case folding. Host names, alert names, and version spellings are
ASCII by definition, and tolower() would consult the locale, which a protocol
module must not.
*/
static char
d_internal_ssl_lower(
    char _c
)
{
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (char)(_c - 'A' + 'a');
    }

    return _c;
}

/*
d_internal_ssl_is_digit
  ASCII decimal digit test, locale-free.
*/
static bool
d_internal_ssl_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/*
d_internal_ssl_is_blank
  The whitespace PEM and hand-typed fingerprints may carry: space, tab, CR, LF.
*/
static bool
d_internal_ssl_is_blank(
    char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\r') ||
             (_c == '\n') );
}

/*
d_internal_ssl_hex_value
  The value of one hex digit of either case, or -1 for anything else.
*/
static int
d_internal_ssl_hex_value(
    char _c
)
{
    if (d_internal_ssl_is_digit(_c))
    {
        return _c - '0';
    }

    if ( (_c >= 'a') &&
         (_c <= 'f') )
    {
        return _c - 'a' + 10;
    }

    if ( (_c >= 'A') &&
         (_c <= 'F') )
    {
        return _c - 'A' + 10;
    }

    return -1;
}

/*
d_internal_ssl_ieq
  ASCII case-insensitive span equality: host names compare this way.
*/
static bool
d_internal_ssl_ieq(
    struct d_pack_text _a,
    struct d_pack_text _b
)
{
    if (_a.length != _b.length)
    {
        return false;
    }

    for (size_t i = 0; i < _a.length; ++i)
    {
        if (d_internal_ssl_lower(_a.data[i]) !=
            d_internal_ssl_lower(_b.data[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_ssl_has_prefix
  Case-insensitive test that _text starts with the NUL-terminated _prefix.
*/
static bool
d_internal_ssl_has_prefix(
    struct d_pack_text _text,
    const char*        _prefix
)
{
    size_t length = strlen(_prefix);

    return ( (_text.length >= length) &&
             (d_internal_ssl_ieq(d_internal_ssl_text(_text.data, length),
                                 d_internal_ssl_text(_prefix, length))) );
}

/*
d_internal_ssl_trim
  Drops blanks from both ends of a span.
*/
static struct d_pack_text
d_internal_ssl_trim(
    struct d_pack_text _text
)
{
    size_t start = 0;
    size_t end   = _text.length;

    while ( (start < end) &&
            (d_internal_ssl_is_blank(_text.data[start])) )
    {
        ++start;
    }

    while ( (end > start) &&
            (d_internal_ssl_is_blank(_text.data[end - 1])) )
    {
        --end;
    }

    return d_internal_ssl_text(_text.data + start, end - start);
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
        return d_internal_ssl_text(_host.data, _host.length - 1);
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
        if (memcmp(_text.data + i, _needle, length) == 0)
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
static enum d_ssl_status
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
        return D_SSL_STATUS_OK;
    }

    // the requirement is already recorded, so a caller can grow and retry
    if ( (_capacity < _reserve) ||
         (_capacity - _reserve < _size) )
    {
        return D_SSL_STATUS_BUFFER_TOO_SMALL;
    }

    return D_SSL_STATUS_OK;
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
        memcpy(_out + *_pos, _data, _length);
        *_pos += _length;
    }

    return;
}


//==============================================================================
// 3.  STATUS
//==============================================================================

/*
d_ssl_status_name
  A switch rather than a table: the values are sparse across two ranges.
*/
const char*
d_ssl_status_name(
    enum d_ssl_status _status
)
{
    switch (_status)
    {
        case D_SSL_STATUS_OK:                return "OK";
        case D_SSL_STATUS_INVALID_ARGUMENT:  return "INVALID_ARGUMENT";
        case D_SSL_STATUS_MALFORMED:         return "MALFORMED";
        case D_SSL_STATUS_NOT_FOUND:         return "NOT_FOUND";
        case D_SSL_STATUS_UNSUPPORTED:       return "UNSUPPORTED";
        case D_SSL_STATUS_INVALID_CONFIG:    return "INVALID_CONFIG";
        case D_SSL_STATUS_WRONG_STATE:       return "WRONG_STATE";
        case D_SSL_STATUS_NOT_TLS:           return "NOT_TLS";
        case D_SSL_STATUS_HANDSHAKE_FAILED:  return "HANDSHAKE_FAILED";
        case D_SSL_STATUS_VERIFY_FAILED:     return "VERIFY_FAILED";
        case D_SSL_STATUS_PROTOCOL_ERROR:    return "PROTOCOL_ERROR";
        case D_SSL_STATUS_UNEXPECTED_EOF:    return "UNEXPECTED_EOF";
        case D_SSL_STATUS_BUFFER_TOO_SMALL:  return "BUFFER_TOO_SMALL";
        case D_SSL_STATUS_WOULD_BLOCK:       return "WOULD_BLOCK";
        case D_SSL_STATUS_TRANSPORT_ERROR:   return "TRANSPORT_ERROR";
        case D_SSL_STATUS_CONNECTION_CLOSED: return "CONNECTION_CLOSED";
        case D_SSL_STATUS_WANT_READ:         return "WANT_READ";
        case D_SSL_STATUS_OUT_OF_MEMORY:     return "OUT_OF_MEMORY";
        case D_SSL_STATUS_BACKEND_ERROR:     return "BACKEND_ERROR";
    }

    return NULL;
}

/*
d_ssl_status_is_formal
  Everything above OK and below the mechanical floor.
*/
bool
d_ssl_status_is_formal(
    enum d_ssl_status _status
)
{
    return ( ((int)_status > (int)D_SSL_STATUS_OK) &&
             ((int)_status < D_SSL_STATUS_MECHANICAL_FLOOR) );
}

/*
d_ssl_status_is_mechanical
  Everything at or above the mechanical floor.
*/
bool
d_ssl_status_is_mechanical(
    enum d_ssl_status _status
)
{
    return ((int)_status >= D_SSL_STATUS_MECHANICAL_FLOOR);
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
    int32_t           alert;
    const char*       name;
    enum d_ssl_status status;
};

// d_internal_ssl_alerts
//   table: every assigned alert, retired ones included, in value order.
static const struct d_internal_ssl_alert_entry d_internal_ssl_alerts[] =
{
    { D_SSL_ALERT_CLOSE_NOTIFY,             "close_notify",
      D_SSL_STATUS_CONNECTION_CLOSED },
    { D_SSL_ALERT_UNEXPECTED_MESSAGE,       "unexpected_message",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_BAD_RECORD_MAC,           "bad_record_mac",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_DECRYPTION_FAILED,        "decryption_failed",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_RECORD_OVERFLOW,          "record_overflow",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_DECOMPRESSION_FAILURE,    "decompression_failure",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_HANDSHAKE_FAILURE,        "handshake_failure",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_NO_CERTIFICATE,           "no_certificate",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_BAD_CERTIFICATE,          "bad_certificate",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_UNSUPPORTED_CERTIFICATE,  "unsupported_certificate",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_CERTIFICATE_REVOKED,      "certificate_revoked",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_CERTIFICATE_EXPIRED,      "certificate_expired",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_CERTIFICATE_UNKNOWN,      "certificate_unknown",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_ILLEGAL_PARAMETER,        "illegal_parameter",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_UNKNOWN_CA,               "unknown_ca",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_ACCESS_DENIED,            "access_denied",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_DECODE_ERROR,             "decode_error",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_DECRYPT_ERROR,            "decrypt_error",
      D_SSL_STATUS_PROTOCOL_ERROR },
    { D_SSL_ALERT_EXPORT_RESTRICTION,       "export_restriction",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_PROTOCOL_VERSION,         "protocol_version",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_INSUFFICIENT_SECURITY,    "insufficient_security",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_INTERNAL_ERROR,           "internal_error",
      D_SSL_STATUS_BACKEND_ERROR },
    { D_SSL_ALERT_INAPPROPRIATE_FALLBACK,   "inappropriate_fallback",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_USER_CANCELED,            "user_canceled",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_NO_RENEGOTIATION,         "no_renegotiation",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_MISSING_EXTENSION,        "missing_extension",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_UNSUPPORTED_EXTENSION,    "unsupported_extension",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_CERTIFICATE_UNOBTAINABLE, "certificate_unobtainable",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_UNRECOGNIZED_NAME,        "unrecognized_name",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE,
      "bad_certificate_status_response",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_BAD_CERTIFICATE_HASH_VALUE,
      "bad_certificate_hash_value",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_UNKNOWN_PSK_IDENTITY,     "unknown_psk_identity",
      D_SSL_STATUS_HANDSHAKE_FAILED },
    { D_SSL_ALERT_CERTIFICATE_REQUIRED,     "certificate_required",
      D_SSL_STATUS_VERIFY_FAILED },
    { D_SSL_ALERT_NO_APPLICATION_PROTOCOL,  "no_application_protocol",
      D_SSL_STATUS_HANDSHAKE_FAILED }
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

    name = d_internal_ssl_trim(_name);

    // the family prefix
    if (d_internal_ssl_has_prefix(name, "tls"))
    {
        pos = 3;
    }
    else if (d_internal_ssl_has_prefix(name, "ssl"))
    {
        ssl = true;
        pos = 3;
    }

    // one separator may follow a prefix
    if ( (pos > 0) &&
         (pos < name.length) &&
         ( (d_internal_ssl_lower(name.data[pos]) == 'v') ||
           (name.data[pos] == ' ') ||
           (name.data[pos] == '_') ) )
    {
        ++pos;
    }

    // the major digit, then an optional minor
    if ( (pos >= name.length) ||
         (!d_internal_ssl_is_digit(name.data[pos])) )
    {
        return D_SSL_VERSION_NONE;
    }

    major = name.data[pos++] - '0';

    if (pos < name.length)
    {
        if ( ( (name.data[pos] != '.') &&
               (name.data[pos] != '_') ) ||
             (pos + 2 != name.length) ||
             (!d_internal_ssl_is_digit(name.data[pos + 1])) )
        {
            return D_SSL_VERSION_NONE;
        }

        minor = name.data[pos + 1] - '0';
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
d_ssl_alert_status
  A table lookup; an unassigned alert is a protocol error, since a conforming
peer does not send one.
*/
enum d_ssl_status
d_ssl_alert_status(
    int32_t _alert
)
{
    const struct d_internal_ssl_alert_entry* entry = NULL;

    if (_alert == D_SSL_ALERT_NONE)
    {
        return D_SSL_STATUS_OK;
    }

    entry = d_internal_ssl_alert_find(_alert);

    return (entry != NULL) ? entry->status : D_SSL_STATUS_PROTOCOL_ERROR;
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
d_ssl_verify_describe
  Unassigned bits fold into OTHER first, so a result never lists "other" twice.
The measure and the write walk the same folded set, so they cannot disagree.
*/
enum d_ssl_status
d_ssl_verify_describe(
    uint32_t _flags,
    char*    _out,
    size_t   _capacity,
    size_t*  _out_size
)
{
    uint32_t          known  = d_internal_ssl_flags_known();
    uint32_t          flags  = _flags & known;
    size_t            size   = 0;
    size_t            pos    = 0;
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if (!d_internal_ssl_out_args_ok(_out, _capacity, _out_size))
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if ((_flags & ~known) != 0)
    {
        flags |= D_SSL_VERIFY_FLAG_OTHER;
    }

    // measure: each phrase, and a separator before all but the first
    if (flags == 0)
    {
        size = 2;
    }

    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        if ((flags & d_internal_ssl_flags[i].flag) != 0)
        {
            if (size > 0)
            {
                size += 2;
            }

            size += strlen(d_internal_ssl_flags[i].name);
        }
    }

    status = d_internal_ssl_out_check(_out, _capacity, size, 1, _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_SSL_STATUS_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    if (flags == 0)
    {
        d_internal_ssl_put(_out, &pos, "ok", 2);
    }

    for (size_t i = 0; i < D_INTERNAL_SSL_FLAG_COUNT; ++i)
    {
        if ((flags & d_internal_ssl_flags[i].flag) != 0)
        {
            if (pos > 0)
            {
                d_internal_ssl_put(_out, &pos, ", ", 2);
            }

            d_internal_ssl_put(_out,
                               &pos,
                               d_internal_ssl_flags[i].name,
                               strlen(d_internal_ssl_flags[i].name));
        }
    }

    _out[pos] = '\0';

    return D_SSL_STATUS_OK;
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
    return ( ( (_c >= 'a') &&
               (_c <= 'z') )                ||
             ( (_c >= 'A') &&
               (_c <= 'Z') )                ||
             (d_internal_ssl_is_digit(_c))  ||
             (_c == '-')                    ||
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
                (d_internal_ssl_is_digit(_text[pos])) )
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
    uint16_t groups[8] = { 0 };
    size_t   count     = 0;
    size_t   gap       = 0;
    bool     has_gap   = false;
    size_t   pos       = 0;

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

        has_gap = true;
        pos     = 2;
    }

    while (pos < _length)
    {
        size_t   start = pos;
        unsigned value = 0;

        // one to four hex digits
        while ( (pos < _length)     &&
                (pos - start < 4)   &&
                (d_internal_ssl_hex_value(_text[pos]) >= 0) )
        {
            value = (value << 4) |
                    (unsigned)d_internal_ssl_hex_value(_text[pos]);
            ++pos;
        }

        // a dot makes this field a dotted-quad tail, which ends the text
        if ( (pos < _length) &&
             (_text[pos] == '.') )
        {
            uint8_t quad[4] = { 0 };

            if ( (count > 6) ||
                 (!d_internal_ssl_ipv4_parse(_text + start,
                                             _length - start,
                                             quad)) )
            {
                return false;
            }

            groups[count++] = (uint16_t)((quad[0] << 8) | quad[1]);
            groups[count++] = (uint16_t)((quad[2] << 8) | quad[3]);

            break;
        }

        // an empty group, a fifth digit, or a ninth group
        if ( (pos == start)                                    ||
             ( (pos < _length) &&
               (d_internal_ssl_hex_value(_text[pos]) >= 0) )   ||
             (count == 8) )
        {
            return false;
        }

        groups[count++] = (uint16_t)value;

        if (pos == _length)
        {
            break;
        }

        // groups are joined by one colon, or once by "::"
        if (_text[pos] != ':')
        {
            return false;
        }

        ++pos;

        if (pos == _length)
        {
            return false;
        }

        if (_text[pos] == ':')
        {
            if (has_gap)
            {
                return false;
            }

            has_gap = true;
            gap     = count;
            ++pos;
        }
    }

    // without "::" all eight groups are spelled; with it, at most seven
    if ( ( (!has_gap) &&
           (count != 8) ) ||
         ( (has_gap) &&
           (count > 7) ) )
    {
        return false;
    }

    // the groups after the gap move to the tail; the gap becomes zeros
    if (has_gap)
    {
        size_t tail = count - gap;

        memmove(&groups[8 - tail], &groups[gap], tail * sizeof(groups[0]));

        for (size_t i = gap; i < 8 - tail; ++i)
        {
            groups[i] = 0;
        }
    }

    for (size_t i = 0; i < 8; ++i)
    {
        _out[2 * i]       = (uint8_t)(groups[i] >> 8);
        _out[(2 * i) + 1] = (uint8_t)(groups[i] & 0xFFu);
    }

    return true;
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
                 (d_ssl_ip_match(_name->value, _host)) );
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

    return (d_ssl_ip_parse(_host, address, &length) == D_SSL_STATUS_OK);
}

/*
d_ssl_ip_parse
  A colon anywhere means IPv6; otherwise the text must be a dotted quad. The
parse goes through scratch storage, so a failure leaves _out untouched.
*/
enum d_ssl_status
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
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if ( (_text.length == 0) ||
         (_text.length > D_SSL_IP_TEXT_MAX) )
    {
        return D_SSL_STATUS_MALFORMED;
    }

    ipv6   = (memchr(_text.data, ':', _text.length) != NULL);
    parsed = ipv6 ? d_internal_ssl_ipv6_parse(_text.data,
                                              _text.length,
                                              address)
                  : d_internal_ssl_ipv4_parse(_text.data,
                                              _text.length,
                                              address);

    if (!parsed)
    {
        return D_SSL_STATUS_MALFORMED;
    }

    *_out_length = ipv6 ? 16u : 4u;
    memcpy(_out, address, *_out_length);

    return D_SSL_STATUS_OK;
}

/*
d_ssl_sni_from_host
  The address test runs on the stripped form, so "192.0.2.1." is still an
address; the name test runs on the original, which may carry the one trailing
dot the stripped form has lost.
*/
enum d_ssl_status
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
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    host = d_internal_ssl_strip_dot(_host);

    // RFC 6066 section 3: address literals are not server names
    if (d_ssl_host_is_ip(host))
    {
        return D_SSL_STATUS_NOT_FOUND;
    }

    if (!d_ssl_hostname_is_valid(_host))
    {
        return D_SSL_STATUS_MALFORMED;
    }

    *_out_sni = host;

    return D_SSL_STATUS_OK;
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

        return d_internal_ssl_ieq(suffix,
                                  d_internal_ssl_text(host.data + skip,
                                                      host.length - skip));
    }

    return ( (d_internal_ssl_name_ok(pattern)) &&
             (d_internal_ssl_ieq(pattern, host)) );
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
         (d_ssl_ip_parse(_host, address, &length) != D_SSL_STATUS_OK) )
    {
        return false;
    }

    return ( (_presented.size == length) &&
             (memcmp(_presented.data, address, length) == 0) );
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
        if (d_internal_ssl_peer_name_matches(&_names[i], _host, is_ip))
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

    while (d_ssl_alpn_next(_list, &offset, &protocol) == D_SSL_STATUS_OK)
    {
        if ( (protocol.length == _protocol.length) &&
             (memcmp(protocol.data, _protocol.data, protocol.length) == 0) )
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
enum d_ssl_status
d_ssl_alpn_encode(
    const struct d_pack_text* _protocols,
    size_t                    _count,
    void*                     _out,
    size_t                    _capacity,
    size_t*                   _out_size
)
{
    unsigned char*    out    = (unsigned char*)_out;
    size_t            size   = 0;
    size_t            pos    = 0;
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if ( (_protocols == NULL) ||
         (_count == 0)        ||
         (!d_internal_ssl_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    // measure, refusing any name the wire format cannot carry
    for (size_t i = 0; i < _count; ++i)
    {
        if (!d_internal_ssl_text_ok(_protocols[i]))
        {
            return D_SSL_STATUS_INVALID_ARGUMENT;
        }

        if ( (_protocols[i].length == 0) ||
             (_protocols[i].length > D_SSL_ALPN_PROTOCOL_MAX) )
        {
            return D_SSL_STATUS_MALFORMED;
        }

        size += 1 + _protocols[i].length;

        if (size > D_SSL_ALPN_LIST_MAX)
        {
            return D_SSL_STATUS_MALFORMED;
        }
    }

    status = d_internal_ssl_out_check(_out, _capacity, size, 0, _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_SSL_STATUS_OK) ||
         (out == NULL) )
    {
        return status;
    }

    for (size_t i = 0; i < _count; ++i)
    {
        out[pos++] = (unsigned char)_protocols[i].length;
        memcpy(out + pos, _protocols[i].data, _protocols[i].length);
        pos += _protocols[i].length;
    }

    return D_SSL_STATUS_OK;
}

/*
d_ssl_alpn_next
  One entry per call. The bounds test is written as a subtraction from the
remaining length, so it cannot overflow however large the offset.
*/
enum d_ssl_status
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
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if (*_offset == _list.size)
    {
        return D_SSL_STATUS_NOT_FOUND;
    }

    length = (unsigned char)data[*_offset];

    // an entry is a length octet and that many name octets, never zero
    if ( (length == 0) ||
         (length > _list.size - *_offset - 1) )
    {
        return D_SSL_STATUS_MALFORMED;
    }

    *_out_protocol = d_internal_ssl_text(data + *_offset + 1, length);
    *_offset      += 1 + length;

    return D_SSL_STATUS_OK;
}

/*
d_ssl_alpn_select
  Both lists are validated whole before either is trusted, which is exactly
the check CVE-2024-5535's selection routine skipped. The server's order
decides (RFC 7301 section 3.2), and the answer points into the client's list.
*/
enum d_ssl_status
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
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if ( (!d_ssl_alpn_is_valid(_preferred)) ||
         (!d_ssl_alpn_is_valid(_offered)) )
    {
        return D_SSL_STATUS_MALFORMED;
    }

    while (d_ssl_alpn_next(_preferred,
                           &preferred_offset,
                           &preferred) == D_SSL_STATUS_OK)
    {
        size_t             offered_offset = 0;
        struct d_pack_text offered        = { NULL, 0 };

        while (d_ssl_alpn_next(_offered,
                               &offered_offset,
                               &offered) == D_SSL_STATUS_OK)
        {
            if ( (offered.length == preferred.length) &&
                 (memcmp(offered.data,
                         preferred.data,
                         offered.length) == 0) )
            {
                *_out_protocol = offered;

                return D_SSL_STATUS_OK;
            }
        }
    }

    return D_SSL_STATUS_NOT_FOUND;
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
        return d_internal_ssl_sniff_ssl2(data, _prefix.size);
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
// 9.  PEM AND BASE64
//==============================================================================

/*
d_internal_ssl_base64_value
  The value of one character of the base64 alphabet, or -1.
*/
static int
d_internal_ssl_base64_value(
    char _c
)
{
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return _c - 'A';
    }

    if ( (_c >= 'a') &&
         (_c <= 'z') )
    {
        return _c - 'a' + 26;
    }

    if (d_internal_ssl_is_digit(_c))
    {
        return _c - '0' + 52;
    }

    if (_c == '+')
    {
        return 62;
    }

    return (_c == '/') ? 63 : -1;
}

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
d_ssl_pem_next
  Finds a BEGIN line, reads its label, and requires that the next boundary be
the END line with the same label: a second BEGIN first, or a different label,
is malformed. On MALFORMED the cursor moves to the end, so a loop over a
damaged bundle terminates.
*/
enum d_ssl_status
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
    size_t             after       = 0;
    struct d_pack_text label       = { NULL, 0 };

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_text)) ||
         (_offset == NULL)                ||
         (_out == NULL)                   ||
         (*_offset > _text.length) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    begin = d_internal_ssl_find(_text, *_offset, "-----BEGIN ");

    if (begin == _text.length)
    {
        *_offset = _text.length;

        return D_SSL_STATUS_NOT_FOUND;
    }

    // the label runs to the dashes that close the BEGIN line
    label_start = begin + 11;
    label_end   = d_internal_ssl_find(_text, label_start, "-----");
    label       = d_internal_ssl_text(_text.data + label_start,
                                      label_end - label_start);

    if ( (label_end == _text.length) ||
         (!d_internal_ssl_pem_label_ok(label)) )
    {
        *_offset = _text.length;

        return D_SSL_STATUS_MALFORMED;
    }

    // the body starts on the line after BEGIN
    body_start = label_end + 5;

    while ( (body_start < _text.length) &&
            ( (_text.data[body_start] == ' ') ||
              (_text.data[body_start] == '\t') ) )
    {
        ++body_start;
    }

    if ( (body_start < _text.length) &&
         (_text.data[body_start] == '\r') )
    {
        ++body_start;
    }

    if ( (body_start < _text.length) &&
         (_text.data[body_start] == '\n') )
    {
        ++body_start;
    }

    // the next boundary must be the matching END line
    end   = d_internal_ssl_find(_text, body_start, "-----END ");
    after = end + 9 + label.length + 5;

    if ( (end == _text.length)                                           ||
         (d_internal_ssl_find(_text, body_start, "-----BEGIN ") < end)   ||
         (after > _text.length)                                          ||
         (memcmp(_text.data + end + 9, label.data, label.length) != 0)   ||
         (memcmp(_text.data + end + 9 + label.length, "-----", 5) != 0) )
    {
        *_offset = _text.length;

        return D_SSL_STATUS_MALFORMED;
    }

    _out->label = label;
    _out->body  = d_internal_ssl_text(_text.data + body_start,
                                      end - body_start);
    *_offset    = after;

    return D_SSL_STATUS_OK;
}

/*
d_ssl_pem_decode
  Two passes. The first counts alphabet characters and padding, refusing
anything after the padding begins, and so fixes the exact size; the second
decodes, stopping at that size so the bits padding stands for are dropped.
*/
enum d_ssl_status
d_ssl_pem_decode(
    struct d_pack_text _body,
    void*              _out,
    size_t             _capacity,
    size_t*            _out_size
)
{
    unsigned char*    out     = (unsigned char*)_out;
    size_t            digits  = 0;
    size_t            padding = 0;
    size_t            size    = 0;
    size_t            pos     = 0;
    uint32_t          bits    = 0;
    unsigned          held    = 0;
    enum d_ssl_status status  = D_SSL_STATUS_OK;

    // parameter validation
    if ( (!d_internal_ssl_text_ok(_body)) ||
         (!d_internal_ssl_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    // count the alphabet and the padding; only blanks may follow padding
    for (size_t i = 0; i < _body.length; ++i)
    {
        char c = _body.data[i];

        if (d_internal_ssl_is_blank(c))
        {
            continue;
        }

        if (c == '=')
        {
            ++padding;

            continue;
        }

        if ( (padding > 0) ||
             (d_internal_ssl_base64_value(c) < 0) )
        {
            return D_SSL_STATUS_MALFORMED;
        }

        ++digits;
    }

    if ( (padding > 2) ||
         ((digits + padding) % 4 != 0) )
    {
        return D_SSL_STATUS_MALFORMED;
    }

    size   = (((digits + padding) / 4) * 3) - padding;
    status = d_internal_ssl_out_check(_out, _capacity, size, 0, _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_SSL_STATUS_OK) ||
         (out == NULL) )
    {
        return status;
    }

    // four characters carry three octets; each full octet is emitted
    for (size_t i = 0; ( (i < _body.length) && (pos < size) ); ++i)
    {
        int value = d_internal_ssl_base64_value(_body.data[i]);

        if (value < 0)
        {
            continue;
        }

        bits  = ((bits << 6) | (uint32_t)value) & 0xFFFFFFu;
        held += 6;

        if (held >= 8)
        {
            held      -= 8;
            out[pos++] = (unsigned char)((bits >> held) & 0xFFu);
        }
    }

    return D_SSL_STATUS_OK;
}


//==============================================================================
// 10. FINGERPRINTS
//==============================================================================

// D_INTERNAL_SSL_SHA256_BLOCK
//   constant: the SHA-256 block size in octets.
#define D_INTERNAL_SSL_SHA256_BLOCK 64

// d_internal_ssl_sha256
//   struct: a running SHA-256 computation (FIPS 180-4): the chaining state,
// the octets hashed so far, and a partial block.
struct d_internal_ssl_sha256
{
    uint32_t state[8];
    uint64_t length;
    size_t   used;
    uint8_t  block[D_INTERNAL_SSL_SHA256_BLOCK];
};

// d_internal_ssl_sha256_k
//   table: the SHA-256 round constants (FIPS 180-4 section 4.2.2).
static const uint32_t d_internal_ssl_sha256_k[64] =
{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

/*
d_internal_ssl_rotr
  32-bit rotate right; _n is always 1..31 here.
*/
static uint32_t
d_internal_ssl_rotr(
    uint32_t _x,
    unsigned _n
)
{
    return (_x >> _n) | (_x << (32u - _n));
}

/*
d_internal_ssl_sha256_compress
  One block through the compression function (FIPS 180-4 section 6.2.2):
expand the message schedule, run 64 rounds, add into the state.
*/
static void
d_internal_ssl_sha256_compress(
    uint32_t      _state[8],
    const uint8_t _block[D_INTERNAL_SSL_SHA256_BLOCK]
)
{
    uint32_t w[64] = { 0 };
    uint32_t v[8]  = { 0 };

    for (size_t i = 0; i < 16; ++i)
    {
        w[i] = ((uint32_t)_block[4 * i] << 24)       |
               ((uint32_t)_block[(4 * i) + 1] << 16) |
               ((uint32_t)_block[(4 * i) + 2] << 8)  |
               (uint32_t)_block[(4 * i) + 3];
    }

    for (size_t i = 16; i < 64; ++i)
    {
        uint32_t s0 = d_internal_ssl_rotr(w[i - 15], 7)  ^
                      d_internal_ssl_rotr(w[i - 15], 18) ^
                      (w[i - 15] >> 3);
        uint32_t s1 = d_internal_ssl_rotr(w[i - 2], 17)  ^
                      d_internal_ssl_rotr(w[i - 2], 19)  ^
                      (w[i - 2] >> 10);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    memcpy(v, _state, sizeof(v));

    // v[0..7] are the working variables a..h
    for (size_t i = 0; i < 64; ++i)
    {
        uint32_t s1  = d_internal_ssl_rotr(v[4], 6)  ^
                       d_internal_ssl_rotr(v[4], 11) ^
                       d_internal_ssl_rotr(v[4], 25);
        uint32_t ch  = (v[4] & v[5]) ^ (~v[4] & v[6]);
        uint32_t t1  = v[7] + s1 + ch + d_internal_ssl_sha256_k[i] + w[i];
        uint32_t s0  = d_internal_ssl_rotr(v[0], 2)  ^
                       d_internal_ssl_rotr(v[0], 13) ^
                       d_internal_ssl_rotr(v[0], 22);
        uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);

        v[7] = v[6];
        v[6] = v[5];
        v[5] = v[4];
        v[4] = v[3] + t1;
        v[3] = v[2];
        v[2] = v[1];
        v[1] = v[0];
        v[0] = t1 + s0 + maj;
    }

    for (size_t i = 0; i < 8; ++i)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_internal_ssl_sha256_init
  The initial hash value (FIPS 180-4 section 5.3.3).
*/
static void
d_internal_ssl_sha256_init(
    struct d_internal_ssl_sha256* _sha
)
{
    static const uint32_t initial[8] =
    {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };

    memset(_sha, 0, sizeof(*_sha));
    memcpy(_sha->state, initial, sizeof(initial));

    return;
}

/*
d_internal_ssl_sha256_update
  Fills the partial block and compresses each one completed.
*/
static void
d_internal_ssl_sha256_update(
    struct d_internal_ssl_sha256* _sha,
    const uint8_t*                _data,
    size_t                        _size
)
{
    size_t pos = 0;

    _sha->length += (uint64_t)_size;

    while (pos < _size)
    {
        size_t take = D_INTERNAL_SSL_SHA256_BLOCK - _sha->used;

        if (take > _size - pos)
        {
            take = _size - pos;
        }

        memcpy(_sha->block + _sha->used, _data + pos, take);
        _sha->used += take;
        pos        += take;

        if (_sha->used == D_INTERNAL_SSL_SHA256_BLOCK)
        {
            d_internal_ssl_sha256_compress(_sha->state, _sha->block);
            _sha->used = 0;
        }
    }

    return;
}

/*
d_internal_ssl_sha256_final
  Pads with a 1 bit and zeros to 56 octets mod 64, appends the bit length
big-endian, and writes the state out big-endian.
*/
static void
d_internal_ssl_sha256_final(
    struct d_internal_ssl_sha256* _sha,
    uint8_t                       _out[D_SSL_FINGERPRINT_LENGTH]
)
{
    uint64_t bits = _sha->length * 8u;

    _sha->block[_sha->used++] = 0x80u;

    // no room for the length: pad this block out and start another
    if (_sha->used > 56)
    {
        memset(_sha->block + _sha->used,
               0,
               D_INTERNAL_SSL_SHA256_BLOCK - _sha->used);
        d_internal_ssl_sha256_compress(_sha->state, _sha->block);
        _sha->used = 0;
    }

    memset(_sha->block + _sha->used, 0, 56 - _sha->used);

    for (size_t i = 0; i < 8; ++i)
    {
        _sha->block[56 + i] = (uint8_t)(bits >> (56u - (8u * i)));
    }

    d_internal_ssl_sha256_compress(_sha->state, _sha->block);

    for (size_t i = 0; i < 8; ++i)
    {
        _out[4 * i]       = (uint8_t)(_sha->state[i] >> 24);
        _out[(4 * i) + 1] = (uint8_t)(_sha->state[i] >> 16);
        _out[(4 * i) + 2] = (uint8_t)(_sha->state[i] >> 8);
        _out[(4 * i) + 3] = (uint8_t)_sha->state[i];
    }

    memset(_sha, 0, sizeof(*_sha));

    return;
}

/*
d_ssl_fingerprint_equal
  Every octet is compared whatever the first difference, so the time taken
says nothing about how much of a pin matched.
*/
bool
d_ssl_fingerprint_equal(
    const uint8_t _a[D_SSL_FINGERPRINT_LENGTH],
    const uint8_t _b[D_SSL_FINGERPRINT_LENGTH]
)
{
    uint8_t difference = 0;

    // parameter validation
    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return false;
    }

    for (size_t i = 0; i < D_SSL_FINGERPRINT_LENGTH; ++i)
    {
        difference = (uint8_t)(difference | (_a[i] ^ _b[i]));
    }

    return (difference == 0);
}

/*
d_ssl_fingerprint_compute
  SHA-256 over the DER, exactly as certificate tools compute it.
*/
enum d_ssl_status
d_ssl_fingerprint_compute(
    struct d_pack_bytes _der,
    uint8_t             _out[D_SSL_FINGERPRINT_LENGTH]
)
{
    struct d_internal_ssl_sha256 sha = { { 0 }, 0, 0, { 0 } };

    // parameter validation
    if ( (!d_internal_ssl_bytes_ok(_der)) ||
         (_out == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    d_internal_ssl_sha256_init(&sha);
    d_internal_ssl_sha256_update(&sha,
                                 (const uint8_t*)_der.data,
                                 _der.size);
    d_internal_ssl_sha256_final(&sha, _out);

    return D_SSL_STATUS_OK;
}

/*
d_ssl_fingerprint_format
  Uppercase pairs with colons between them, and a NUL.
*/
enum d_ssl_status
d_ssl_fingerprint_format(
    const uint8_t _fingerprint[D_SSL_FINGERPRINT_LENGTH],
    char          _out[D_SSL_FINGERPRINT_TEXT_LENGTH + 1]
)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t            pos   = 0;

    // parameter validation
    if ( (_fingerprint == NULL) ||
         (_out == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < D_SSL_FINGERPRINT_LENGTH; ++i)
    {
        if (i > 0)
        {
            _out[pos++] = ':';
        }

        _out[pos++] = hex[_fingerprint[i] >> 4];
        _out[pos++] = hex[_fingerprint[i] & 0x0Fu];
    }

    _out[pos] = '\0';

    return D_SSL_STATUS_OK;
}

/*
d_ssl_fingerprint_parse
  Tool output is cut at its last '=' first; what remains must be exactly 64
contiguous digits or 32 colon-joined pairs, so a truncated or padded paste is
refused rather than half-read.
*/
enum d_ssl_status
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
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    text = d_internal_ssl_trim(_text);

    // "sha256 Fingerprint=AB:CD:..." keeps only what follows the '='
    for (size_t i = text.length; i > 0; --i)
    {
        if (text.data[i - 1] == '=')
        {
            text = d_internal_ssl_trim(
                       d_internal_ssl_text(text.data + i, text.length - i));

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
        return D_SSL_STATUS_MALFORMED;
    }

    for (size_t i = 0; i < D_SSL_FINGERPRINT_LENGTH; ++i)
    {
        size_t at   = i * stride;
        int    high = d_internal_ssl_hex_value(text.data[at]);
        int    low  = d_internal_ssl_hex_value(text.data[at + 1]);

        if ( (high < 0)                                  ||
             (low < 0)                                   ||
             ( (stride == 3)                          &&
               (i + 1 < D_SSL_FINGERPRINT_LENGTH)     &&
               (text.data[at + 2] != ':') ) )
        {
            return D_SSL_STATUS_MALFORMED;
        }

        digest[i] = (uint8_t)((high << 4) | low);
    }

    memcpy(_out, digest, sizeof(digest));

    return D_SSL_STATUS_OK;
}


//==============================================================================
// 11. TRUST STORES
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
// 12. CONFIGURATION
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
unknown minimum, or a maximum below it, is INVALID_CONFIG.
*/
static enum d_ssl_status
d_internal_ssl_versions_ok(
    const struct d_ssl_config* _config
)
{
    int minimum = (int)_config->min_version;
    int maximum = (int)_config->max_version;

    if ( (minimum == (int)D_SSL_VERSION_SSL3) ||
         (maximum == (int)D_SSL_VERSION_SSL3) )
    {
        return D_SSL_STATUS_UNSUPPORTED;
    }

    if ( (!d_ssl_version_is_known(_config->min_version)) ||
         ( (maximum != (int)D_SSL_VERSION_NONE) &&
           ( (!d_ssl_version_is_known(_config->max_version)) ||
             (maximum < minimum) ) ) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // RFC 8996's deprecation holds unless the build opted out of it
    if ( (!D_INTERNAL_SSL_ALLOW_LEGACY) &&
         (minimum < (int)D_SSL_VERSION_TLS1_2) )
    {
        return D_SSL_STATUS_UNSUPPORTED;
    }

    return D_SSL_STATUS_OK;
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

    memset(_config, 0, sizeof(*_config));

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
enum d_ssl_status
d_ssl_config_add_pin(
    struct d_ssl_config* _config,
    const uint8_t        _fingerprint[D_SSL_FINGERPRINT_LENGTH]
)
{
    // parameter validation
    if ( (_config == NULL) ||
         (_fingerprint == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if (_config->pin_count >= D_SSL_PINS_MAX)
    {
        return D_SSL_STATUS_BUFFER_TOO_SMALL;
    }

    memcpy(_config->pins[_config->pin_count],
           _fingerprint,
           D_SSL_FINGERPRINT_LENGTH);
    ++_config->pin_count;

    return D_SSL_STATUS_OK;
}

/*
d_ssl_config_validate
  The rules in the header, in its order. Spans that break their own contract
are caller bugs and INVALID_ARGUMENT; everything else is INVALID_CONFIG,
except what the build refuses outright, which is UNSUPPORTED.
*/
enum d_ssl_status
d_ssl_config_validate(
    const struct d_ssl_config* _config
)
{
    enum d_ssl_status status      = D_SSL_STATUS_OK;
    bool              verifies    = false;
    bool              has_anchors = false;

    // parameter validation
    if ( (_config == NULL)                                  ||
         (!d_internal_ssl_text_ok(_config->ca_pem))         ||
         (!d_internal_ssl_text_ok(_config->certificate_pem)) ||
         (!d_internal_ssl_text_ok(_config->private_key_pem)) ||
         (!d_internal_ssl_bytes_ok(_config->alpn)) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if ( (_config->role != D_SSL_ROLE_CLIENT) &&
         (_config->role != D_SSL_ROLE_SERVER) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    status = d_internal_ssl_versions_ok(_config);

    if (status != D_SSL_STATUS_OK)
    {
        return status;
    }

    // verification modes, and OPTIONAL being a server's alone
    if ( ( (_config->verify != D_SSL_VERIFY_NONE)     &&
           (_config->verify != D_SSL_VERIFY_OPTIONAL) &&
           (_config->verify != D_SSL_VERIFY_REQUIRED) ) ||
         ( (_config->verify == D_SSL_VERIFY_OPTIONAL) &&
           (_config->role == D_SSL_ROLE_CLIENT) ) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // verifying against a chain needs something to anchor it
    verifies    = (_config->verify != D_SSL_VERIFY_NONE);
    has_anchors = ( (_config->use_system_trust)   ||
                    (_config->ca_file != NULL)    ||
                    (_config->ca_path != NULL)    ||
                    (_config->ca_pem.length > 0) );

    if ( (verifies)            &&
         (!_config->pin_only)  &&
         (!has_anchors) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    if ( (!d_internal_ssl_identity_ok(_config))                     ||
         (!d_internal_ssl_path_ok(_config->ca_file))                ||
         (!d_internal_ssl_path_ok(_config->ca_path))                ||
         (!d_internal_ssl_path_ok(_config->certificate_file))       ||
         (!d_internal_ssl_path_ok(_config->private_key_file))       ||
         (!d_internal_ssl_cipher_ok(_config->cipher_list))          ||
         (!d_internal_ssl_cipher_ok(_config->cipher_suites)) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // an ALPN list must be well-formed, and a requirement needs a list
    if ( ( (_config->alpn.size > 0) &&
           (!d_ssl_alpn_is_valid(_config->alpn)) ) ||
         ( (_config->alpn_required) &&
           (_config->alpn.size == 0) ) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // pins fit, pin_only has some, and they only mean something when verifying
    if ( (_config->pin_count > D_SSL_PINS_MAX)     ||
         ( (_config->pin_only) &&
           (_config->pin_count == 0) )             ||
         ( (_config->pin_count > 0) &&
           (!verifies) ) )
    {
        return D_SSL_STATUS_INVALID_CONFIG;
    }

    // a key log leaks session secrets, so a build must have asked for it
    if ( (_config->keylog != NULL) &&
         (!D_INTERNAL_SSL_KEYLOG) )
    {
        return D_SSL_STATUS_UNSUPPORTED;
    }

    return D_SSL_STATUS_OK;
}

/*
d_ssl_config_plan
  The whole of the configuration's meaning for a handshake, derived in one
place so no engine derives it differently.
*/
enum d_ssl_status
d_ssl_config_plan(
    const struct d_ssl_config* _config,
    struct d_ssl_plan*         _out_plan
)
{
    struct d_ssl_plan plan     = { D_SSL_ROLE_CLIENT, D_SSL_VERSION_NONE,
                                   D_SSL_VERSION_NONE, false, false, false,
                                   false, false, false, false };
    enum d_ssl_status status   = D_SSL_STATUS_OK;
    bool              verifies = false;
    bool              server   = false;

    // parameter validation
    if ( (_config == NULL) ||
         (_out_plan == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    status = d_ssl_config_validate(_config);

    if (status != D_SSL_STATUS_OK)
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

    return D_SSL_STATUS_OK;
}


//==============================================================================
// 13. TRANSPORT
//==============================================================================

/*
d_internal_ssl_side_ok
  A pipe has ends A and B and nothing else.
*/
static bool
d_internal_ssl_side_ok(
    enum d_ssl_pipe_side _side
)
{
    return ( (_side == D_SSL_PIPE_A) ||
             (_side == D_SSL_PIPE_B) );
}

/*
d_internal_ssl_ring_take
  Copies _count bytes, at most the ring's size, from its head, wrapping as
needed.
*/
static void
d_internal_ssl_ring_take(
    struct d_ssl_ring* _ring,
    unsigned char*     _out,
    size_t             _count
)
{
    size_t first = _ring->capacity - _ring->head;

    if (first > _count)
    {
        first = _count;
    }

    memcpy(_out, _ring->data + _ring->head, first);
    memcpy(_out + first, _ring->data, _count - first);

    _ring->head  = (_ring->head + _count) % _ring->capacity;
    _ring->size -= _count;

    return;
}

/*
d_internal_ssl_ring_put
  Copies _count bytes, at most the ring's free space, to its tail, wrapping as
needed.
*/
static void
d_internal_ssl_ring_put(
    struct d_ssl_ring*   _ring,
    const unsigned char* _data,
    size_t               _count
)
{
    size_t tail  = (_ring->head + _ring->size) % _ring->capacity;
    size_t first = _ring->capacity - tail;

    if (first > _count)
    {
        first = _count;
    }

    memcpy(_ring->data + tail, _data, first);
    memcpy(_ring->data, _data + first, _count - first);

    _ring->size += _count;

    return;
}

/*
d_internal_ssl_pipe_read
  A pipe end's read callback: whatever its inbound ring holds, up to the
capacity and the end's chunk limit. An empty ring is the end of the stream
once its writer has closed, and "not yet" until then.
*/
static enum d_ssl_status
d_internal_ssl_pipe_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    struct d_ssl_pipe_end* end   = (struct d_ssl_pipe_end*)_context;
    size_t                 count = 0;

    // parameter validation
    if ( (end == NULL)      ||
         (_buffer == NULL)  ||
         (_capacity == 0)   ||
         (_out_read == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    *_out_read = 0;

    if (end->inbound->size == 0)
    {
        return (end->inbound->closed) ? D_SSL_STATUS_CONNECTION_CLOSED
                                      : D_SSL_STATUS_WOULD_BLOCK;
    }

    count = (_capacity < end->inbound->size) ? _capacity
                                             : end->inbound->size;

    if ( (end->chunk > 0) &&
         (count > end->chunk) )
    {
        count = end->chunk;
    }

    d_internal_ssl_ring_take(end->inbound, (unsigned char*)_buffer, count);
    *_out_read = count;

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_pipe_write
  A pipe end's write callback: as much as its outbound ring has room for, up
to the end's chunk limit. A full ring is "not yet"; a closed one refuses.
*/
static enum d_ssl_status
d_internal_ssl_pipe_write(
    void*       _context,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    struct d_ssl_pipe_end* end   = (struct d_ssl_pipe_end*)_context;
    size_t                 space = 0;
    size_t                 count = 0;

    // parameter validation
    if ( (end == NULL)    ||
         (_data == NULL)  ||
         (_size == 0)     ||
         (_out_written == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    *_out_written = 0;

    if (end->outbound->closed)
    {
        return D_SSL_STATUS_TRANSPORT_ERROR;
    }

    space = end->outbound->capacity - end->outbound->size;

    if (space == 0)
    {
        return D_SSL_STATUS_WOULD_BLOCK;
    }

    count = (_size < space) ? _size : space;

    if ( (end->chunk > 0) &&
         (count > end->chunk) )
    {
        count = end->chunk;
    }

    d_internal_ssl_ring_put(end->outbound,
                            (const unsigned char*)_data,
                            count);
    *_out_written = count;

    return D_SSL_STATUS_OK;
}

/*
d_ssl_pipe_init
  Ring 0 carries A to B and ring 1 carries B to A; each end reads one and
writes the other.
*/
enum d_ssl_status
d_ssl_pipe_init(
    struct d_ssl_pipe* _pipe,
    void*              _buffer_a,
    size_t             _capacity_a,
    void*              _buffer_b,
    size_t             _capacity_b
)
{
    // parameter validation
    if ( (_pipe == NULL)       ||
         (_buffer_a == NULL)   ||
         (_capacity_a == 0)    ||
         (_buffer_b == NULL)   ||
         (_capacity_b == 0) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    memset(_pipe, 0, sizeof(*_pipe));

    _pipe->rings[0].data     = (unsigned char*)_buffer_a;
    _pipe->rings[0].capacity = _capacity_a;
    _pipe->rings[1].data     = (unsigned char*)_buffer_b;
    _pipe->rings[1].capacity = _capacity_b;

    _pipe->ends[D_SSL_PIPE_A].inbound  = &_pipe->rings[1];
    _pipe->ends[D_SSL_PIPE_A].outbound = &_pipe->rings[0];
    _pipe->ends[D_SSL_PIPE_B].inbound  = &_pipe->rings[0];
    _pipe->ends[D_SSL_PIPE_B].outbound = &_pipe->rings[1];

    return D_SSL_STATUS_OK;
}

/*
d_ssl_pipe_transport
  The end's callbacks, with the end itself as their context.
*/
struct d_ssl_transport
d_ssl_pipe_transport(
    struct d_ssl_pipe*   _pipe,
    enum d_ssl_pipe_side _side
)
{
    struct d_ssl_transport transport = { NULL, NULL, NULL };

    if ( (_pipe == NULL) ||
         (!d_internal_ssl_side_ok(_side)) )
    {
        return transport;
    }

    transport.read    = d_internal_ssl_pipe_read;
    transport.write   = d_internal_ssl_pipe_write;
    transport.context = &_pipe->ends[_side];

    return transport;
}

/*
d_ssl_pipe_set_chunk
  Zero removes the limit.
*/
void
d_ssl_pipe_set_chunk(
    struct d_ssl_pipe*   _pipe,
    enum d_ssl_pipe_side _side,
    size_t               _chunk
)
{
    if ( (_pipe != NULL) &&
         (d_internal_ssl_side_ok(_side)) )
    {
        _pipe->ends[_side].chunk = _chunk;
    }

    return;
}

/*
d_ssl_pipe_close
  Half-closes: the end's outbound ring is marked, and the ring itself tells its
reader once drained.
*/
void
d_ssl_pipe_close(
    struct d_ssl_pipe*   _pipe,
    enum d_ssl_pipe_side _side
)
{
    if ( (_pipe != NULL) &&
         (d_internal_ssl_side_ok(_side)) &&
         (_pipe->ends[_side].outbound != NULL) )
    {
        _pipe->ends[_side].outbound->closed = true;
    }

    return;
}

/*
d_ssl_pipe_pending
  The fill level of the ring the end reads.
*/
size_t
d_ssl_pipe_pending(
    const struct d_ssl_pipe* _pipe,
    enum d_ssl_pipe_side     _side
)
{
    if ( (_pipe == NULL)                    ||
         (!d_internal_ssl_side_ok(_side))   ||
         (_pipe->ends[_side].inbound == NULL) )
    {
        return 0;
    }

    return _pipe->ends[_side].inbound->size;
}

/*
d_ssl_transport_write_all
  Loops over short writes. A transport reporting no progress, or more than it
was given, is broken rather than slow, and looping on it would never end.
*/
enum d_ssl_status
d_ssl_transport_write_all(
    struct d_ssl_transport _transport,
    const void*            _data,
    size_t                 _size,
    size_t*                _out_written
)
{
    const unsigned char* data   = (const unsigned char*)_data;
    size_t               done   = 0;
    enum d_ssl_status    status = D_SSL_STATUS_OK;

    // parameter validation
    if ( (_transport.write == NULL) ||
         ( (_data == NULL) &&
           (_size > 0) ) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    while (done < _size)
    {
        size_t written = 0;

        status = _transport.write(_transport.context,
                                  data + done,
                                  _size - done,
                                  &written);

        if (status != D_SSL_STATUS_OK)
        {
            break;
        }

        if ( (written == 0) ||
             (written > _size - done) )
        {
            status = D_SSL_STATUS_TRANSPORT_ERROR;

            break;
        }

        done += written;
    }

    if (_out_written != NULL)
    {
        *_out_written = done;
    }

    return status;
}


//==============================================================================
// 14. CONTEXTS
//==============================================================================

/*
d_internal_ssl_engine_status
  Holds an engine to its contract. OK, the known formal statuses,
OUT_OF_MEMORY, and BACKEND_ERROR pass as they are; so do WANT_READ and
CONNECTION_CLOSED where the operation may return them. Anything else --
WOULD_BLOCK and TRANSPORT_ERROR above all, which only a transport may report
-- becomes BACKEND_ERROR, so a misbehaving engine can neither make the kernel
retry forever nor pass itself off as the network.
*/
static enum d_ssl_status
d_internal_ssl_engine_status(
    enum d_ssl_status _status,
    bool              _may_want,
    bool              _may_close
)
{
    if ( (_status == D_SSL_STATUS_OK)                         ||
         ( (d_ssl_status_is_formal(_status)) &&
           (d_ssl_status_name(_status) != NULL) )             ||
         (_status == D_SSL_STATUS_OUT_OF_MEMORY)              ||
         (_status == D_SSL_STATUS_BACKEND_ERROR)              ||
         ( (_may_want) &&
           (_status == D_SSL_STATUS_WANT_READ) )              ||
         ( (_may_close) &&
           (_status == D_SSL_STATUS_CONNECTION_CLOSED) ) )
    {
        return _status;
    }

    return D_SSL_STATUS_BACKEND_ERROR;
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
static enum d_ssl_status
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
        return D_SSL_STATUS_UNSUPPORTED;
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
        return D_SSL_STATUS_UNSUPPORTED;
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
        return D_SSL_STATUS_UNSUPPORTED;
    }

    // names and pins are the kernel's to check, with the engine's help
    if ( ( (_plan->verify_hostname)                         &&
           (_engine->peer_name == NULL)                     &&
           ((features & D_SSL_ENGINE_HOSTNAME) == 0) )      ||
         ( (_plan->check_pins) &&
           (_engine->peer_certificate == NULL) ) )
    {
        return D_SSL_STATUS_UNSUPPORTED;
    }

    return D_SSL_STATUS_OK;
}

/*
d_ssl_context_init
  Plan, fit, create. The context is filled only once the engine's context
exists, so every failure leaves it empty.
*/
enum d_ssl_status
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
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if (_context == NULL)
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    memset(_context, 0, sizeof(*_context));

    if ( (_engine == NULL) ||
         (_config == NULL) ||
         (!d_internal_ssl_engine_complete(_engine)) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    status = d_ssl_config_plan(_config, &plan);

    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_fit_engine(_config, _engine, &plan);
    }

    if (status == D_SSL_STATUS_OK)
    {
        status = d_internal_ssl_engine_status(
                     _engine->context_create(_config, &plan, &handle),
                     false,
                     false);
    }

    if (status != D_SSL_STATUS_OK)
    {
        return status;
    }

    _context->engine    = _engine;
    _context->handle    = handle;
    _context->plan      = plan;
    _context->pin_count = _config->pin_count;
    memcpy(_context->pins, _config->pins, sizeof(_context->pins));

    return D_SSL_STATUS_OK;
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

    memset(_context, 0, sizeof(*_context));

    return;
}

//==============================================================================
// 15. SESSIONS
//==============================================================================

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
d_internal_ssl_info_reset
  What is known before anything is negotiated: no version, no alerts, and a
peer not yet authenticated.
*/
static void
d_internal_ssl_info_reset(
    struct d_ssl_info* _info
)
{
    memset(_info, 0, sizeof(*_info));

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
    _session->context->engine->describe(_session->handle, &fresh);

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
static enum d_ssl_status
d_internal_ssl_fail(
    struct d_ssl_session* _session,
    enum d_ssl_status     _status
)
{
    _session->state   = D_SSL_STATE_FAILED;
    _session->failure = _status;
    _session->want    = D_SSL_WANT_NOTHING;

    return _status;
}

/*
d_internal_ssl_flush
  Moves ciphertext from the engine to the transport through the stage, until
the engine has nothing left or the transport stops taking bytes. The stage is
refilled only once empty, so bytes leave in the order the engine produced
them however the transport splits its writes. Returns the transport's
WOULD_BLOCK (noting that the session waits to write) or CONNECTION_CLOSED
as they are, for the caller to judge, and any other trouble as
TRANSPORT_ERROR or BACKEND_ERROR.
*/
static enum d_ssl_status
d_internal_ssl_flush(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_status          status = D_SSL_STATUS_OK;

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

            if ( (status != D_SSL_STATUS_OK) ||
                 (moved > sizeof(_session->stage)) )
            {
                return D_SSL_STATUS_BACKEND_ERROR;
            }

            if (moved == 0)
            {
                return D_SSL_STATUS_OK;
            }

            _session->stage_end = moved;
            pending             = moved;
            moved               = 0;
        }

        status = _session->transport.write(
                     _session->transport.context,
                     _session->stage + _session->stage_start,
                     pending,
                     &moved);

        if (status == D_SSL_STATUS_WOULD_BLOCK)
        {
            _session->want = D_SSL_WANT_WRITE;

            return status;
        }

        if (status == D_SSL_STATUS_CONNECTION_CLOSED)
        {
            return status;
        }

        // any other answer, or a count outside the contract, is breakage
        if ( (status != D_SSL_STATUS_OK) ||
             (moved == 0)                ||
             (moved > pending) )
        {
            return D_SSL_STATUS_TRANSPORT_ERROR;
        }

        _session->stage_start += moved;
        _session->bytes_out   += moved;
    }
}

/*
d_internal_ssl_sniff_input
  Collects the peer's first bytes, up to a record header's worth, and judges
them. Once they are judged TLS the session stops looking; NOT_TLS ends it
before the engine has parsed a byte of whatever the peer is speaking.
*/
static enum d_ssl_status
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

    memcpy(_session->sniff + _session->sniff_length, _data, take);
    _session->sniff_length += take;

    switch (d_ssl_sniff(d_internal_ssl_bytes(_session->sniff,
                                             _session->sniff_length),
                        NULL))
    {
        case D_SSL_SNIFF_NOT_TLS:
            return D_SSL_STATUS_NOT_TLS;

        case D_SSL_SNIFF_TLS:
        case D_SSL_SNIFF_SSL2:
            _session->sniffed = true;
            break;

        case D_SSL_SNIFF_NEED_MORE:
            break;
    }

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_pull
  One transport read, sniffed if the session is still judging its peer, then
fed to the engine whole. Returns the transport's WOULD_BLOCK (noting that the
session waits to read) or CONNECTION_CLOSED as they are, for the caller to
judge.
*/
static enum d_ssl_status
d_internal_ssl_pull(
    struct d_ssl_session* _session
)
{
    unsigned char     chunk[D_SSL_IO_CAPACITY] = { 0 };
    size_t            count                    = 0;
    enum d_ssl_status status                   = D_SSL_STATUS_OK;

    status = _session->transport.read(_session->transport.context,
                                      chunk,
                                      sizeof(chunk),
                                      &count);

    if (status == D_SSL_STATUS_WOULD_BLOCK)
    {
        _session->want = D_SSL_WANT_READ;

        return status;
    }

    if (status == D_SSL_STATUS_CONNECTION_CLOSED)
    {
        return status;
    }

    // any other answer, or a count outside the contract, is breakage
    if ( (status != D_SSL_STATUS_OK) ||
         (count == 0)                ||
         (count > sizeof(chunk)) )
    {
        return D_SSL_STATUS_TRANSPORT_ERROR;
    }

    _session->bytes_in += count;

    if (!_session->sniffed)
    {
        status = d_internal_ssl_sniff_input(_session, chunk, count);

        if (status != D_SSL_STATUS_OK)
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
static enum d_ssl_status
d_internal_ssl_await(
    struct d_ssl_session* _session
)
{
    enum d_ssl_status status = d_internal_ssl_flush(_session);

    if (status != D_SSL_STATUS_OK)
    {
        return status;
    }

    return d_internal_ssl_pull(_session);
}

/*
d_internal_ssl_io_failure
  Judges what flush, pull, or await returned. WOULD_BLOCK is not a failure:
the caller retries later. A transport that ends, in either direction, before
the TLS stream has is UNEXPECTED_EOF -- never CONNECTION_CLOSED, which is
reserved for close_notify, so truncation cannot pass for a clean end.
Everything else fails the session as it is.
*/
static enum d_ssl_status
d_internal_ssl_io_failure(
    struct d_ssl_session* _session,
    enum d_ssl_status     _status
)
{
    if (_status == D_SSL_STATUS_WOULD_BLOCK)
    {
        return _status;
    }

    if (_status == D_SSL_STATUS_CONNECTION_CLOSED)
    {
        return d_internal_ssl_fail(_session, D_SSL_STATUS_UNEXPECTED_EOF);
    }

    return d_internal_ssl_fail(_session, _status);
}

/*
d_internal_ssl_fail_engine
  An engine step failed. Its info is captured for the caller, the alert it
queued is sent if the transport takes it at once, and the session fails.
*/
static enum d_ssl_status
d_internal_ssl_fail_engine(
    struct d_ssl_session* _session,
    enum d_ssl_status     _status
)
{
    d_internal_ssl_capture(_session);

    // best effort: a blocked or broken transport just loses the alert
    (void)d_internal_ssl_flush(_session);

    return d_internal_ssl_fail(_session, _status);
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
        if (!engine->peer_name(_session->handle, i, &name))
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
d_internal_ssl_check_peer
  The kernel's checks on a finished engine handshake, run while its last
flight is still inside the engine. The engine enforced the plan within the
handshake; these confirm the outcome, and add the checks the kernel makes for
every engine alike. The verification result is settled here, and a failure
is VERIFY_FAILED with the flags saying why.
*/
static enum d_ssl_status
d_internal_ssl_check_peer(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_context* context = _session->context;
    const struct d_ssl_plan*    plan    = &context->plan;
    const struct d_ssl_engine*  engine  = context->engine;
    struct d_ssl_info*          info    = &_session->info;
    struct d_pack_bytes         der     = { NULL, 0 };
    bool                        present = false;
    uint32_t                    verify  = D_SSL_VERIFY_FLAG_NONE;

    d_internal_ssl_capture(_session);

    // the version window, which a correct engine never breaches
    if ( (!d_ssl_version_is_known(info->version))              ||
         ((int)info->version < (int)plan->min_version)         ||
         ((int)info->version > (int)plan->max_version) )
    {
        return D_SSL_STATUS_HANDSHAKE_FAILED;
    }

    if ( (plan->require_alpn) &&
         (info->alpn_length == 0) )
    {
        return D_SSL_STATUS_HANDSHAKE_FAILED;
    }

    // the peer certificate, fingerprinted here whichever engine produced it
    if (engine->peer_certificate != NULL)
    {
        present = ( (engine->peer_certificate(_session->handle, &der)) &&
                    (der.data != NULL)                                 &&
                    (der.size > 0) );

        if ( (present) &&
             (d_ssl_fingerprint_compute(der, info->fingerprint) ==
              D_SSL_STATUS_OK) )
        {
            info->has_peer_certificate = true;
        }
    }
    else
    {
        present = ((info->verify & D_SSL_VERIFY_FLAG_NO_CERTIFICATE) == 0);
    }

    // a peer no one asked to authenticate is recorded as such
    if ( (!plan->verify_chain) &&
         (!plan->check_pins) )
    {
        info->verify = D_SSL_VERIFY_FLAG_NOT_PERFORMED;

        return D_SSL_STATUS_OK;
    }

    // without a certificate: fatal if required, else an unauthenticated peer
    if (!present)
    {
        info->verify = D_SSL_VERIFY_FLAG_NO_CERTIFICATE;

        if (plan->require_certificate)
        {
            return D_SSL_STATUS_VERIFY_FAILED;
        }

        info->verify |= D_SSL_VERIFY_FLAG_NOT_PERFORMED;

        return D_SSL_STATUS_OK;
    }

    // the engine's chain verdict, unless pins alone establish trust
    if (plan->verify_chain)
    {
        verify |= info->verify;
    }

    if ( (plan->check_pins) &&
         (!d_internal_ssl_pins_match(context, info->fingerprint)) )
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

    info->verify = verify;

    return (verify == D_SSL_VERIFY_FLAG_NONE) ? D_SSL_STATUS_OK
                                              : D_SSL_STATUS_VERIFY_FAILED;
}

/*
d_internal_ssl_handshake
  Drives the engine's handshake. Each round flushes what the engine queued,
steps it, and, when it waits on the peer, flushes again and reads. When the
engine reports completion the kernel's checks run while the last flight is
still inside the engine, so a peer that fails them never receives it. The
session is ESTABLISHED only once that flight is on the wire; engine_done and
checked let a WOULD_BLOCK anywhere resume at the right step.
*/
static enum d_ssl_status
d_internal_ssl_handshake(
    struct d_ssl_session* _session
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_status          status = D_SSL_STATUS_OK;

    _session->state = D_SSL_STATE_HANDSHAKING;

    while (!_session->engine_done)
    {
        // what the engine queued goes out before it is asked for more
        status = d_internal_ssl_flush(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_io_failure(_session, status);
        }

        status = d_internal_ssl_engine_status(
                     engine->handshake(_session->handle),
                     true,
                     true);

        // a close_notify before the handshake finished is a refusal
        if (status == D_SSL_STATUS_CONNECTION_CLOSED)
        {
            status = D_SSL_STATUS_HANDSHAKE_FAILED;
        }

        if (status == D_SSL_STATUS_OK)
        {
            _session->engine_done = true;

            break;
        }

        if (status != D_SSL_STATUS_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session, status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_io_failure(_session, status);
        }
    }

    // the checks see the finished handshake before its last flight leaves
    if (!_session->checked)
    {
        status = d_internal_ssl_check_peer(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_fail(_session, status);
        }

        _session->checked = true;
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_STATUS_OK)
    {
        return d_internal_ssl_io_failure(_session, status);
    }

    _session->state = D_SSL_STATE_ESTABLISHED;
    _session->want  = D_SSL_WANT_NOTHING;

    return D_SSL_STATUS_OK;
}

/*
d_internal_ssl_read
  The read pump of an established session. A WANT_READ may follow records
the engine absorbed without yielding data -- session tickets, key updates --
so the loop runs until data, the peer's close_notify, or a failure.
*/
static enum d_ssl_status
d_internal_ssl_read(
    struct d_ssl_session* _session,
    void*                 _buffer,
    size_t                _capacity,
    size_t*               _out_read
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_status          status = D_SSL_STATUS_OK;

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

        if (status == D_SSL_STATUS_OK)
        {
            if ( (count == 0) ||
                 (count > _capacity) )
            {
                return d_internal_ssl_fail(_session,
                                           D_SSL_STATUS_BACKEND_ERROR);
            }

            *_out_read = count;

            // replies the read provoked go out now if the transport allows;
            // a failure here resurfaces on the next call, not this one
            status         = d_internal_ssl_flush(_session);
            _session->want = (status == D_SSL_STATUS_WOULD_BLOCK)
                                 ? D_SSL_WANT_WRITE
                                 : D_SSL_WANT_NOTHING;

            return D_SSL_STATUS_OK;
        }

        if (status == D_SSL_STATUS_CONNECTION_CLOSED)
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

        if (status != D_SSL_STATUS_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session, status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_io_failure(_session, status);
        }
    }
}

/*
d_internal_ssl_write
  The write pump of an established session with nothing pending. Once the
engine has taken data it is committed; a transport that then blocks delays
its ciphertext but does not undo it.
*/
static enum d_ssl_status
d_internal_ssl_write(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    const struct d_ssl_engine* engine = _session->context->engine;
    enum d_ssl_status          status = D_SSL_STATUS_OK;

    for (;;)
    {
        size_t taken = 0;

        status = d_internal_ssl_engine_status(
                     engine->write(_session->handle, _data, _size, &taken),
                     true,
                     false);

        if (status == D_SSL_STATUS_OK)
        {
            if ( (taken == 0) ||
                 (taken > _size) )
            {
                return d_internal_ssl_fail(_session,
                                           D_SSL_STATUS_BACKEND_ERROR);
            }

            *_out_written = taken;
            status        = d_internal_ssl_flush(_session);

            // committed data waits in the session; wants() says WRITE
            if (status == D_SSL_STATUS_WOULD_BLOCK)
            {
                return D_SSL_STATUS_OK;
            }

            if (status != D_SSL_STATUS_OK)
            {
                return d_internal_ssl_io_failure(_session, status);
            }

            _session->want = D_SSL_WANT_NOTHING;

            return D_SSL_STATUS_OK;
        }

        if (status != D_SSL_STATUS_WANT_READ)
        {
            return d_internal_ssl_fail_engine(_session, status);
        }

        status = d_internal_ssl_await(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_io_failure(_session, status);
        }
    }
}

/*
d_ssl_session_init
  The record is cleared first, so every early return leaves it empty. The host
is validated and copied without its trailing dot; the engine receives it as
the reference identity only when the plan checks names, and as SNI only when
it is a name rather than an address.
*/
enum d_ssl_status
d_ssl_session_init(
    struct d_ssl_session*       _session,
    const struct d_ssl_context* _context,
    struct d_ssl_transport      _transport,
    struct d_pack_text          _host
)
{
    struct d_pack_text host   = { NULL, 0 };
    const char*        name   = NULL;
    const char*        sni    = NULL;
    void*              handle = NULL;
    enum d_ssl_status  status = D_SSL_STATUS_OK;

    // parameter validation
    if (_session == NULL)
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    memset(_session, 0, sizeof(*_session));

    if ( (_context == NULL)                 ||
         (_context->engine == NULL)         ||
         (_transport.read == NULL)          ||
         (_transport.write == NULL)         ||
         (!d_internal_ssl_text_ok(_host)) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    host = d_internal_ssl_strip_dot(_host);

    // a server names no host; a client that checks names must name one
    if ( ( (_context->plan.role == D_SSL_ROLE_SERVER) &&
           (_host.length > 0) )                         ||
         ( (_context->plan.verify_hostname) &&
           (host.length == 0) ) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    // a host is an address literal or a valid name
    if (_host.length > 0)
    {
        _session->host_is_ip = d_ssl_host_is_ip(host);

        if ( (!_session->host_is_ip) &&
             (!d_ssl_hostname_is_valid(_host)) )
        {
            _session->host_is_ip = false;

            return D_SSL_STATUS_MALFORMED;
        }

        memcpy(_session->host, host.data, host.length);
        _session->host[host.length] = '\0';
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

    if (status != D_SSL_STATUS_OK)
    {
        memset(_session, 0, sizeof(*_session));

        return status;
    }

    _session->context   = _context;
    _session->handle    = handle;
    _session->transport = _transport;
    _session->state     = D_SSL_STATE_IDLE;
    _session->failure   = D_SSL_STATUS_OK;
    _session->want      = D_SSL_WANT_NOTHING;
    d_internal_ssl_info_reset(&_session->info);

    return D_SSL_STATUS_OK;
}

/*
d_ssl_session_destroy
  Hands the engine's session back and clears the record, stage included.
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

    memset(_session, 0, sizeof(*_session));

    return;
}

/*
d_ssl_session_handshake
  Dispatches on the state; the work is d_internal_ssl_handshake's.
*/
enum d_ssl_status
d_ssl_session_handshake(
    struct d_ssl_session* _session
)
{
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    switch (_session->state)
    {
        case D_SSL_STATE_FAILED:
            return _session->failure;

        case D_SSL_STATE_CLOSED:
            return D_SSL_STATUS_WRONG_STATE;

        case D_SSL_STATE_ESTABLISHED:
            status = d_internal_ssl_flush(_session);

            if (status != D_SSL_STATUS_OK)
            {
                return d_internal_ssl_io_failure(_session, status);
            }

            _session->want = D_SSL_WANT_NOTHING;

            return D_SSL_STATUS_OK;

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
enum d_ssl_status
d_ssl_session_read(
    struct d_ssl_session* _session,
    void*                 _buffer,
    size_t                _capacity,
    size_t*               _out_read
)
{
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         (_buffer == NULL)                      ||
         (_capacity == 0)                       ||
         (_out_read == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    *_out_read = 0;

    if (_session->state == D_SSL_STATE_FAILED)
    {
        return _session->failure;
    }

    if (_session->received_close)
    {
        return D_SSL_STATUS_CONNECTION_CLOSED;
    }

    if (_session->state == D_SSL_STATE_CLOSED)
    {
        return D_SSL_STATUS_WRONG_STATE;
    }

    // a read handshakes first, as a transport's user expects
    if (_session->state != D_SSL_STATE_ESTABLISHED)
    {
        status = d_internal_ssl_handshake(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return status;
        }
    }

    return d_internal_ssl_read(_session, _buffer, _capacity, _out_read);
}

/*
d_ssl_session_write
  Earlier ciphertext goes first, and new data is taken only once it has gone:
a session never holds more than one write's worth unsent.
*/
enum d_ssl_status
d_ssl_session_write(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         (_data == NULL)                        ||
         (_size == 0)                           ||
         (_out_written == NULL) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
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
        return D_SSL_STATUS_WRONG_STATE;
    }

    if (_session->state != D_SSL_STATE_ESTABLISHED)
    {
        status = d_internal_ssl_handshake(_session);

        if (status != D_SSL_STATUS_OK)
        {
            return status;
        }
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_STATUS_OK)
    {
        return d_internal_ssl_io_failure(_session, status);
    }

    return d_internal_ssl_write(_session, _data, _size, _out_written);
}

/*
d_ssl_session_write_all
  Stops at the first status other than OK, reporting how far it got; a write
that blocked after taking data returns OK with the rest pending, so the loop
meets the blockage on its next write and reports it there.
*/
enum d_ssl_status
d_ssl_session_write_all(
    struct d_ssl_session* _session,
    const void*           _data,
    size_t                _size,
    size_t*               _out_written
)
{
    const unsigned char* data   = (const unsigned char*)_data;
    size_t               done   = 0;
    enum d_ssl_status    status = D_SSL_STATUS_OK;

    // parameter validation
    if ( (!d_internal_ssl_session_ok(_session)) ||
         ( (_data == NULL) &&
           (_size > 0) ) )
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    while (done < _size)
    {
        size_t taken = 0;

        status = d_ssl_session_write(_session,
                                     data + done,
                                     _size - done,
                                     &taken);
        done  += taken;

        if (status != D_SSL_STATUS_OK)
        {
            break;
        }
    }

    if (_out_written != NULL)
    {
        *_out_written = done;
    }

    if (status != D_SSL_STATUS_OK)
    {
        return status;
    }

    return d_ssl_session_flush(_session);
}

/*
d_ssl_session_flush
  Valid in any state but FAILED; an idle session simply has nothing to send.
*/
enum d_ssl_status
d_ssl_session_flush(
    struct d_ssl_session* _session
)
{
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    if (_session->state == D_SSL_STATE_FAILED)
    {
        return _session->failure;
    }

    status = d_internal_ssl_flush(_session);

    if (status != D_SSL_STATUS_OK)
    {
        return d_internal_ssl_io_failure(_session, status);
    }

    _session->want = D_SSL_WANT_NOTHING;

    return D_SSL_STATUS_OK;
}

/*
d_ssl_session_shutdown
  close_notify is queued once, behind anything already pending, and flushed.
A peer that closed first may already have gone; nothing further is owed to
it, so its departure is not an error here, and what could not be sent is
dropped.
*/
enum d_ssl_status
d_ssl_session_shutdown(
    struct d_ssl_session* _session
)
{
    enum d_ssl_status status = D_SSL_STATUS_OK;

    // parameter validation
    if (!d_internal_ssl_session_ok(_session))
    {
        return D_SSL_STATUS_INVALID_ARGUMENT;
    }

    switch (_session->state)
    {
        case D_SSL_STATE_FAILED:
            return _session->failure;

        case D_SSL_STATE_CLOSED:
            return D_SSL_STATUS_OK;

        // nothing established means nothing to close: just stop
        case D_SSL_STATE_IDLE:
        case D_SSL_STATE_HANDSHAKING:
            _session->state = D_SSL_STATE_CLOSED;
            _session->want  = D_SSL_WANT_NOTHING;

            return D_SSL_STATUS_OK;

        case D_SSL_STATE_ESTABLISHED:
            break;
    }

    if (!_session->sent_close)
    {
        status = d_internal_ssl_engine_status(
                     _session->context->engine->close(_session->handle),
                     false,
                     false);

        if (status != D_SSL_STATUS_OK)
        {
            return d_internal_ssl_fail_engine(_session, status);
        }

        _session->sent_close = true;
    }

    status = d_internal_ssl_flush(_session);

    if ( (status == D_SSL_STATUS_CONNECTION_CLOSED) &&
         (_session->received_close) )
    {
        _session->stage_start = 0;
        _session->stage_end   = 0;
        status                = D_SSL_STATUS_OK;
    }

    if (status != D_SSL_STATUS_OK)
    {
        return d_internal_ssl_io_failure(_session, status);
    }

    if (_session->received_close)
    {
        _session->state = D_SSL_STATE_CLOSED;
    }

    _session->want = D_SSL_WANT_NOTHING;

    return D_SSL_STATUS_OK;
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
d_internal_ssl_session_read_fn
  d_ssl_session_read in the shape of a transport read callback.
*/
static enum d_ssl_status
d_internal_ssl_session_read_fn(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    return d_ssl_session_read((struct d_ssl_session*)_context,
                              _buffer,
                              _capacity,
                              _out_read);
}

/*
d_internal_ssl_session_write_fn
  d_ssl_session_write in the shape of a transport write callback.
*/
static enum d_ssl_status
d_internal_ssl_session_write_fn(
    void*       _context,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    return d_ssl_session_write((struct d_ssl_session*)_context,
                               _data,
                               _size,
                               _out_written);
}

/*
d_ssl_session_transport
  The session's plaintext as a transport: its reads and writes, with the
session as their context.
*/
struct d_ssl_transport
d_ssl_session_transport(
    struct d_ssl_session* _session
)
{
    struct d_ssl_transport transport = { NULL, NULL, NULL };

    if (_session == NULL)
    {
        return transport;
    }

    transport.read    = d_internal_ssl_session_read_fn;
    transport.write   = d_internal_ssl_session_write_fn;
    transport.context = _session;

    return transport;
}
