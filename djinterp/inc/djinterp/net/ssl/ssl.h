/*******************************************************************************
* djinterp [net]                                                           ssl.h
*
* The shared SSL/TLS kernel: tier 0 of the SSL modules.
*   Everything TLS clients, servers, and test doubles share that needs no
* TLS library, plus the contract through which a library is plugged in:
*     - status codes, and the protocol vocabulary as data           [2, 3]
*     - peer-verification policy and results                           [4]
*     - RFC 9525 identity matching, IP literals, and SNI               [5]
*     - ALPN lists, and telling TLS from plaintext by its first bytes  [6]
*     - PEM, base64, and SHA-256 certificate fingerprints              [7]
*     - configuration, validated and resolved into a plan              [8]
*     - a transport interface, and an in-memory pipe for tests         [9]
*     - the sans-I/O engine interface a TLS library is bound through  [10]
*     - contexts and sessions: the pump and the kernel's own checks   [11]
*   THE KERNEL LINKS NO TLS LIBRARY AND NEVER OPENS A SOCKET. An engine -- a
* derived module wrapping OpenSSL or another library -- turns ciphertext
* into plaintext and back. The kernel moves every byte between it and a
* d_ssl_transport, and checks names, pins, versions, and ALPN itself, so a
* session behaves the same under every engine. The kernel allocates
* nothing; engines allocate.
*   TLS may begin on a transport that has already carried plaintext, which
* is what STARTTLS-style upgrades build on; the upgrade commands themselves
* belong to the protocol modules that speak them.
*   Text spans are d_pack_text and byte spans d_pack_bytes, from
* sink_common.h. Every declaration has C linkage.
*   Build configuration comes from cfg_ssl.h; environment detection for the
* engines from env_ssl.h.
*
*
* path:      /inc/djinterp/net/ssl/ssl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PROTOCOL CONSTANTS
    ------------------
    1.  Record layer
         1.  D_SSL_RECORD_HEADER_SIZE
         2.  D_SSL_RECORD_PLAINTEXT_MAX
         3.  D_SSL_RECORD_CIPHERTEXT_MAX
    2.  Names and identities
         1.  D_SSL_HOSTNAME_MAX
         2.  D_SSL_LABEL_MAX
         3.  D_SSL_IP_TEXT_MAX
         4.  D_SSL_IP_MAX
         5.  D_SSL_ALPN_PROTOCOL_MAX
         6.  D_SSL_ALPN_LIST_MAX
    3.  Fingerprints and pins
         1.  D_SSL_FINGERPRINT_LENGTH
         2.  D_SSL_FINGERPRINT_TEXT_LENGTH
         3.  D_SSL_PINS_MAX
    4.  Kernel buffers
         1.  D_SSL_IO_CAPACITY
         2.  D_SSL_CIPHER_NAME_MAX
2.  RESULT STATUS
    -------------
    1.  Status codes
         1.  d_ssl_status
         2.  D_SSL_STATUS_MECHANICAL_FLOOR
    2.  Status queries
3.  PROTOCOL VOCABULARY
    -------------------
    1.  Versions
         1.  d_ssl_version
         2.  D_SSL_VERSION_NEWEST
         3.  D_SSL_VERSION_FLOOR
    2.  Roles and backends
         1.  d_ssl_role
         2.  d_ssl_backend
    3.  Records and alerts
         1.  d_ssl_content_type
         2.  d_ssl_alert
         3.  D_SSL_ALERT_NONE
    4.  Vocabulary operations
4.  PEER VERIFICATION
    -----------------
    1.  Policy and results
         1.  d_ssl_verify_mode
         2.  d_ssl_verify_flag
    2.  Verification operations
5.  IDENTITIES
    ----------
    1.  Presented identifiers
         1.  d_ssl_name_kind
         2.  d_ssl_peer_name
    2.  Host names and addresses
    3.  Identity matching
6.  WIRE HELPERS
    ------------
    1.  Application-layer protocol negotiation
    2.  Stream sniffing
         1.  d_ssl_sniff_result
         2.  d_ssl_record_header
    3.  Sniffing operations
7.  CERTIFICATES AND TRUST
    ----------------------
    1.  PEM
         1.  d_ssl_pem_block
    2.  PEM operations
    3.  Fingerprints
    4.  Platform trust stores
8.  CONFIGURATION
    -------------
    1.  Configuration records
         1.  d_ssl_keylog_fn
         2.  d_ssl_config
         3.  d_ssl_plan
    2.  Configuration operations
9.  TRANSPORT
    ---------
    1.  The transport interface
         1.  d_ssl_transport_read_fn
         2.  d_ssl_transport_write_fn
         3.  d_ssl_transport
    2.  The in-memory pipe
         1.  d_ssl_ring
         2.  d_ssl_pipe_end
         3.  d_ssl_pipe
         4.  d_ssl_pipe_side
    3.  Transport operations
10. ENGINES
    -------
    1.  Engine vocabulary
         1.  d_ssl_engine_feature
         2.  d_ssl_info
    2.  Engine operations
         1.  d_ssl_engine_context_create_fn
         2.  d_ssl_engine_context_destroy_fn
         3.  d_ssl_engine_session_create_fn
         4.  d_ssl_engine_session_destroy_fn
         5.  d_ssl_engine_feed_fn
         6.  d_ssl_engine_drain_fn
         7.  d_ssl_engine_step_fn
         8.  d_ssl_engine_read_fn
         9.  d_ssl_engine_write_fn
         10. d_ssl_engine_describe_fn
         11. d_ssl_engine_peer_name_fn
         12. d_ssl_engine_peer_certificate_fn
    3.  The engine interface
         1.  d_ssl_engine
11. CONTEXTS AND SESSIONS
    ---------------------
    1.  Contexts
         1.  d_ssl_context
    2.  Context operations
    3.  Sessions
         1.  d_ssl_state
         2.  d_ssl_want
         3.  d_ssl_session
    4.  Session operations
*/

#ifndef DJINTERP_NET_SSL_SSL_H
#define DJINTERP_NET_SSL_SSL_H 1

// std
#include <stddef.h>                        // size_t
#include <stdint.h>                        // uint8_t, uint16_t, uint32_t
// djinterp
#include "../../c/djinterp.h"              // framework root
#include "../../c/util/sink_common.h"      // d_pack_text, d_pack_bytes
#include "../../config/net/ssl/cfg_ssl.h"  // D_INTERNAL_SSL_*

#if !defined(D_EXTERN_C_BEGIN) || !defined(D_EXTERN_C_END)
    #error "ssl.h requires D_EXTERN_C_BEGIN/D_EXTERN_C_END from djinterp.h"
#endif

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PROTOCOL CONSTANTS
//==============================================================================
// Numbers the specifications fix -- RFC 8446 (TLS 1.3), RFC 5246 (TLS 1.2),
// RFC 1035 (host names), RFC 7301 (ALPN) -- and the two sizes cfg_ssl.h
// chooses. None depends on a TLS library.


// 1.1    Record layer
//------------------------------------------------------------------------------
// 1.1.1
// D_SSL_RECORD_HEADER_SIZE
//   constant: the size of a TLS record header: type, version, and length.
#define D_SSL_RECORD_HEADER_SIZE      5

// 1.1.2
// D_SSL_RECORD_PLAINTEXT_MAX
//   constant: the most plaintext one record carries, 2^14 (RFC 8446 section
// 5.1).
#define D_SSL_RECORD_PLAINTEXT_MAX    16384

// 1.1.3
// D_SSL_RECORD_CIPHERTEXT_MAX
//   constant: the longest record body any version permits, 2^14 + 2048 (RFC
// 5246 section 6.2.3). TLS 1.3 narrows it to 2^14 + 256.
#define D_SSL_RECORD_CIPHERTEXT_MAX   18432

// 1.2    Names and identities
//------------------------------------------------------------------------------
// 1.2.1
// D_SSL_HOSTNAME_MAX
//   constant: the longest host name in characters, excluding a trailing dot
// (RFC 1035 section 2.3.4).
#define D_SSL_HOSTNAME_MAX            253

// 1.2.2
// D_SSL_LABEL_MAX
//   constant: the longest label of a host name.
#define D_SSL_LABEL_MAX               63

// 1.2.3
// D_SSL_IP_TEXT_MAX
//   constant: the longest IP address literal: an IPv6 address with an IPv4
// tail, "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255".
#define D_SSL_IP_TEXT_MAX             45

// 1.2.4
// D_SSL_IP_MAX
//   constant: the size of a binary IP address -- 16 octets for IPv6; IPv4
// uses 4.
#define D_SSL_IP_MAX                  16

// 1.2.5
// D_SSL_ALPN_PROTOCOL_MAX
//   constant: the longest ALPN protocol name, in octets (RFC 7301 section
// 3.1).
#define D_SSL_ALPN_PROTOCOL_MAX       255

// 1.2.6
// D_SSL_ALPN_LIST_MAX
//   constant: the longest encoded ALPN protocol list, in octets.
#define D_SSL_ALPN_LIST_MAX           65535

// 1.3    Fingerprints and pins
//------------------------------------------------------------------------------
// 1.3.1
// D_SSL_FINGERPRINT_LENGTH
//   constant: the size of a certificate fingerprint, a SHA-256 digest.
#define D_SSL_FINGERPRINT_LENGTH      32

// 1.3.2
// D_SSL_FINGERPRINT_TEXT_LENGTH
//   constant: the length of a formatted fingerprint, "AB:CD:...:EF",
// excluding its NUL.
#define D_SSL_FINGERPRINT_TEXT_LENGTH 95

// 1.3.3
// D_SSL_PINS_MAX
//   constant: how many fingerprints one configuration may pin (cfg_ssl.h).
#define D_SSL_PINS_MAX                D_INTERNAL_SSL_PINS_MAX

// 1.4    Kernel buffers
//------------------------------------------------------------------------------
// 1.4.1
// D_SSL_IO_CAPACITY
//   constant: the size of a session's outbound staging buffer, and of the
// stack buffer each transport read goes through (cfg_ssl.h).
#define D_SSL_IO_CAPACITY             D_INTERNAL_SSL_IO_BUFFER

// 1.4.2
// D_SSL_CIPHER_NAME_MAX
//   constant: the capacity of d_ssl_info's cipher-suite name, NUL included.
// The longest registered suite name is well under it.
#define D_SSL_CIPHER_NAME_MAX         64


//==============================================================================
// 2.  RESULT STATUS
//==============================================================================
// Two kinds of failure, kept apart as in the other kernels. A FORMAL failure
// says the TLS exchange, the peer, or the configuration is at fault: the same
// call fails the same way again. A MECHANICAL failure says the machinery ran
// short -- a buffer, a transport that would block or broke, memory, the
// library itself -- and the exchange may be sound. The ranges are disjoint,
// so "may I retry?" is a range test.
//   The first four mechanical values equal their d_pop_status namesakes on
// purpose: a module carrying one net kernel over another, POP3 over a TLS
// session say, passes transport statuses across unchanged.


// 2.1    Status codes
//------------------------------------------------------------------------------
// 2.1.1
// d_ssl_status
//   enum: the result of every fallible operation in this module. Values are
// pinned. The less obvious ones:
//     NOT_FOUND          a search came up empty: no further PEM block, no ALPN
//                        protocol in common, no server name for an address.
//     NOT_TLS            the peer's first bytes are not a TLS record: it is
//                        speaking something else, usually plaintext.
//     UNEXPECTED_EOF     the transport ended before the TLS stream did: the
//                        peer hung up mid-handshake, or data may be missing.
//     CONNECTION_CLOSED  the orderly end: the peer sent close_notify.
//     WANT_READ          an engine needs more ciphertext. It passes only from
//                        an engine to the kernel; no session call returns it.
enum d_ssl_status
{
    D_SSL_STATUS_OK                = 0,

    // formal: the exchange, the peer, or the configuration is at fault
    D_SSL_STATUS_INVALID_ARGUMENT  = 0x001,
    D_SSL_STATUS_MALFORMED         = 0x002,
    D_SSL_STATUS_NOT_FOUND         = 0x003,
    D_SSL_STATUS_UNSUPPORTED       = 0x004,
    D_SSL_STATUS_INVALID_CONFIG    = 0x005,
    D_SSL_STATUS_WRONG_STATE       = 0x006,
    D_SSL_STATUS_NOT_TLS           = 0x007,
    D_SSL_STATUS_HANDSHAKE_FAILED  = 0x008,
    D_SSL_STATUS_VERIFY_FAILED     = 0x009,
    D_SSL_STATUS_PROTOCOL_ERROR    = 0x00A,
    D_SSL_STATUS_UNEXPECTED_EOF    = 0x00B,

    // mechanical: the machinery ran short
    D_SSL_STATUS_BUFFER_TOO_SMALL  = 0x100,
    D_SSL_STATUS_WOULD_BLOCK       = 0x101,
    D_SSL_STATUS_TRANSPORT_ERROR   = 0x102,
    D_SSL_STATUS_CONNECTION_CLOSED = 0x103,
    D_SSL_STATUS_WANT_READ         = 0x104,
    D_SSL_STATUS_OUT_OF_MEMORY     = 0x105,
    D_SSL_STATUS_BACKEND_ERROR     = 0x106
};

// 2.1.2
// D_SSL_STATUS_MECHANICAL_FLOOR
//   constant: the lowest mechanical status value.
#define D_SSL_STATUS_MECHANICAL_FLOOR 0x100

// 2.2    Status queries
//------------------------------------------------------------------------------
// Pure lookups; they never fail. d_ssl_status_name returns the enumerator's
// name without its prefix ("VERIFY_FAILED"), or NULL for a value outside the
// enum. The two predicates classify a status by range; OK is neither.
const char* d_ssl_status_name(enum d_ssl_status _status);
bool        d_ssl_status_is_formal(enum d_ssl_status _status);
bool        d_ssl_status_is_mechanical(enum d_ssl_status _status);


//==============================================================================
// 3.  PROTOCOL VOCABULARY
//==============================================================================
// The names TLS gives things: protocol versions (as their wire code points),
// the two roles, the libraries an engine may wrap, record content types, and
// alerts. Engines translate their library's constants into these, so an
// application reads one vocabulary whichever library runs underneath.


// 3.1    Versions
//------------------------------------------------------------------------------
// 3.1.1
// d_ssl_version
//   enum: a protocol version, valued by its wire code point. SSL 3.0 is named
// so it can be reported and refused; it is never negotiated (RFC 7568), and
// SSL 2.0 is not named at all.
enum d_ssl_version
{
    D_SSL_VERSION_NONE   = 0,
    D_SSL_VERSION_SSL3   = 0x0300,
    D_SSL_VERSION_TLS1_0 = 0x0301,
    D_SSL_VERSION_TLS1_1 = 0x0302,
    D_SSL_VERSION_TLS1_2 = 0x0303,
    D_SSL_VERSION_TLS1_3 = 0x0304
};

// 3.1.2
// D_SSL_VERSION_NEWEST
//   constant: the newest version the kernel knows. A configuration whose
// maximum is D_SSL_VERSION_NONE resolves to it, and engines may lower it.
#define D_SSL_VERSION_NEWEST D_SSL_VERSION_TLS1_3

// 3.1.3
// D_SSL_VERSION_FLOOR
//   constant: the default minimum version, TLS 1.2, per RFC 8996 and
// RFC 9325.
#define D_SSL_VERSION_FLOOR  D_SSL_VERSION_TLS1_2

// 3.2    Roles and backends
//------------------------------------------------------------------------------
// 3.2.1
// d_ssl_role
//   enum: which end of the handshake a context plays. Zero is not a role, so
// a configuration left zeroed fails validation instead of guessing.
enum d_ssl_role
{
    D_SSL_ROLE_CLIENT = 1,
    D_SSL_ROLE_SERVER = 2
};

// 3.2.2
// d_ssl_backend
//   enum: the TLS library behind an engine. Values equal env_tls.h's
// D_ENV_TLS_BACKEND_* identifiers, so a run-time answer compares with a
// compile-time one; OTHER covers test doubles and out-of-tree engines.
enum d_ssl_backend
{
    D_SSL_BACKEND_NONE            = 0,
    D_SSL_BACKEND_OPENSSL         = 1,
    D_SSL_BACKEND_MBEDTLS         = 2,
    D_SSL_BACKEND_GNUTLS          = 3,
    D_SSL_BACKEND_WOLFSSL         = 4,
    D_SSL_BACKEND_SCHANNEL        = 5,
    D_SSL_BACKEND_SECURETRANSPORT = 6,
    D_SSL_BACKEND_OTHER           = 0x7F
};

// 3.3    Records and alerts
//------------------------------------------------------------------------------
// 3.3.1
// d_ssl_content_type
//   enum: the content type in a record header (RFC 8446 section 5.1).
enum d_ssl_content_type
{
    D_SSL_CONTENT_CHANGE_CIPHER_SPEC = 20,
    D_SSL_CONTENT_ALERT              = 21,
    D_SSL_CONTENT_HANDSHAKE          = 22,
    D_SSL_CONTENT_APPLICATION_DATA   = 23,
    D_SSL_CONTENT_HEARTBEAT          = 24
};

// 3.3.2
// d_ssl_alert
//   enum: alert descriptions (RFC 8446 section 6). Values retired by TLS 1.3
// are kept, since a TLS 1.2 peer may still send them; they are marked below.
enum d_ssl_alert
{
    D_SSL_ALERT_CLOSE_NOTIFY                    = 0,
    D_SSL_ALERT_UNEXPECTED_MESSAGE              = 10,
    D_SSL_ALERT_BAD_RECORD_MAC                  = 20,
    D_SSL_ALERT_DECRYPTION_FAILED               = 21,   // retired
    D_SSL_ALERT_RECORD_OVERFLOW                 = 22,
    D_SSL_ALERT_DECOMPRESSION_FAILURE           = 30,   // retired
    D_SSL_ALERT_HANDSHAKE_FAILURE               = 40,
    D_SSL_ALERT_NO_CERTIFICATE                  = 41,   // retired
    D_SSL_ALERT_BAD_CERTIFICATE                 = 42,
    D_SSL_ALERT_UNSUPPORTED_CERTIFICATE         = 43,
    D_SSL_ALERT_CERTIFICATE_REVOKED             = 44,
    D_SSL_ALERT_CERTIFICATE_EXPIRED             = 45,
    D_SSL_ALERT_CERTIFICATE_UNKNOWN             = 46,
    D_SSL_ALERT_ILLEGAL_PARAMETER               = 47,
    D_SSL_ALERT_UNKNOWN_CA                      = 48,
    D_SSL_ALERT_ACCESS_DENIED                   = 49,
    D_SSL_ALERT_DECODE_ERROR                    = 50,
    D_SSL_ALERT_DECRYPT_ERROR                   = 51,
    D_SSL_ALERT_EXPORT_RESTRICTION              = 60,   // retired
    D_SSL_ALERT_PROTOCOL_VERSION                = 70,
    D_SSL_ALERT_INSUFFICIENT_SECURITY           = 71,
    D_SSL_ALERT_INTERNAL_ERROR                  = 80,
    D_SSL_ALERT_INAPPROPRIATE_FALLBACK          = 86,
    D_SSL_ALERT_USER_CANCELED                   = 90,
    D_SSL_ALERT_NO_RENEGOTIATION                = 100,  // retired
    D_SSL_ALERT_MISSING_EXTENSION               = 109,
    D_SSL_ALERT_UNSUPPORTED_EXTENSION           = 110,
    D_SSL_ALERT_CERTIFICATE_UNOBTAINABLE        = 111,  // retired
    D_SSL_ALERT_UNRECOGNIZED_NAME               = 112,
    D_SSL_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE = 113,
    D_SSL_ALERT_BAD_CERTIFICATE_HASH_VALUE      = 114,  // retired
    D_SSL_ALERT_UNKNOWN_PSK_IDENTITY            = 115,
    D_SSL_ALERT_CERTIFICATE_REQUIRED            = 116,
    D_SSL_ALERT_NO_APPLICATION_PROTOCOL         = 120
};

// 3.3.3
// D_SSL_ALERT_NONE
//   constant: "no alert", for the alert fields of d_ssl_info. Alert values
// are carried as int32_t there so this can sit outside the 0..255 range.
#define D_SSL_ALERT_NONE (-1)

// 3.4    Vocabulary operations
//------------------------------------------------------------------------------
// Pure lookups; they never fail.
//   d_ssl_version_name gives the conventional log spelling -- "TLSv1.3",
// "SSLv3" -- or NULL for NONE or an unknown value. d_ssl_version_is_known is
// true for SSL3 through TLS1_3; d_ssl_version_is_deprecated is true for SSL3,
// TLS1_0, and TLS1_1 (RFC 8996). d_ssl_version_from_name reads the spellings
// found in configuration files, case-insensitively -- "TLSv1.2", "TLS1.2",
// "tls 1.2", "TLS_1_2", "1.2", "SSLv3" -- and returns NONE for anything else.
//   d_ssl_alert_name gives the RFC name ("close_notify"), or NULL for an
// unassigned value or D_SSL_ALERT_NONE. d_ssl_alert_status says what an alert
// means for a session: close_notify is CONNECTION_CLOSED; certificate
// complaints are VERIFY_FAILED; failures to agree on parameters are
// HANDSHAKE_FAILED; internal_error is BACKEND_ERROR; the rest, unknown values
// included, are PROTOCOL_ERROR. D_SSL_ALERT_NONE maps to OK.
const char*        d_ssl_version_name(enum d_ssl_version _version);
bool               d_ssl_version_is_known(enum d_ssl_version _version);
bool               d_ssl_version_is_deprecated(enum d_ssl_version _version);
enum d_ssl_version d_ssl_version_from_name(struct d_pack_text _name);
const char*        d_ssl_role_name(enum d_ssl_role _role);
const char*        d_ssl_backend_name(enum d_ssl_backend _backend);
const char*        d_ssl_alert_name(int32_t _alert);
enum d_ssl_status  d_ssl_alert_status(int32_t _alert);


//==============================================================================
// 4.  PEER VERIFICATION
//==============================================================================
// What a configuration asks of the peer, and what the session learned. The
// policy is one of three modes. The result is a set of flags, one per reason
// a peer can fail: the engine reports what its library found in the chain,
// and the kernel adds what it checks itself -- the host name, pinned
// fingerprints, certificate presence -- so a result reads the same whichever
// library produced it.


// 4.1    Policy and results
//------------------------------------------------------------------------------
// 4.1.1
// d_ssl_verify_mode
//   enum: how hard to check the peer.
//     NONE      accept any peer, unauthenticated. Servers default to this, as
//               they seldom ask clients for certificates.
//     OPTIONAL  server only: ask for a client certificate and verify it if
//               one arrives, but proceed without one.
//     REQUIRED  verify the peer, failing the handshake without a valid
//               certificate. Clients default to this.
enum d_ssl_verify_mode
{
    D_SSL_VERIFY_NONE     = 0,
    D_SSL_VERIFY_OPTIONAL = 1,
    D_SSL_VERIFY_REQUIRED = 2
};

// 4.1.2
// d_ssl_verify_flag
//   enum: bits of a verification result (d_ssl_info.verify). A result of
// D_SSL_VERIFY_FLAG_NONE means every check performed passed. NOT_PERFORMED
// marks a peer that was never authenticated -- verification off, or an
// optional client certificate that did not arrive -- and is not a failure by
// itself. OTHER stands for any library reason without a flag of its own.
enum d_ssl_verify_flag
{
    D_SSL_VERIFY_FLAG_NONE              = 0x0000,
    D_SSL_VERIFY_FLAG_NOT_PERFORMED     = 0x0001,
    D_SSL_VERIFY_FLAG_NO_CERTIFICATE    = 0x0002,
    D_SSL_VERIFY_FLAG_UNTRUSTED         = 0x0004,
    D_SSL_VERIFY_FLAG_SELF_SIGNED       = 0x0008,
    D_SSL_VERIFY_FLAG_EXPIRED           = 0x0010,
    D_SSL_VERIFY_FLAG_NOT_YET_VALID     = 0x0020,
    D_SSL_VERIFY_FLAG_REVOKED           = 0x0040,
    D_SSL_VERIFY_FLAG_BAD_SIGNATURE     = 0x0080,
    D_SSL_VERIFY_FLAG_WRONG_PURPOSE     = 0x0100,
    D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH = 0x0200,
    D_SSL_VERIFY_FLAG_PIN_MISMATCH      = 0x0400,
    D_SSL_VERIFY_FLAG_OTHER             = 0x8000
};

// 4.2    Verification operations
//------------------------------------------------------------------------------
// Pure lookups; they never fail. d_ssl_verify_mode_name gives "none",
// "optional", or "required". d_ssl_verify_flag_name gives a short phrase for
// a single flag ("hostname mismatch"), or NULL for zero, several bits, or an
// unassigned bit.
const char* d_ssl_verify_mode_name(enum d_ssl_verify_mode _mode);
const char* d_ssl_verify_flag_name(uint32_t _flag);

/**
 * @brief Renders a verification result as a comma-separated list of phrases.
 *
 * @note Flags are listed lowest bit first -- "untrusted issuer, hostname
 *       mismatch" -- and a result of zero renders as "ok". Unassigned bits
 *       render once, as "other". The text is NUL-terminated for logging.
 *       Two-call protocol: with `_out` NULL and `_capacity` 0, only the
 *       length is stored.
 *
 * @param[in]  _flags     the result, a set of d_ssl_verify_flag bits.
 * @param[out] _out       the destination, or NULL to measure.
 * @param[in]  _capacity  the size of `_out` in bytes; it must exceed the
 *                        text length.
 * @param[out] _out_size  receives the text length, excluding the NUL.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_BUFFER_TOO_SMALL, with `*_out_size`
 *         set, if the text and its NUL do not fit; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for a NULL `_out_size`, or a NULL
 *         `_out` with a nonzero `_capacity`.
 */
enum d_ssl_status d_ssl_verify_describe(uint32_t _flags,
                                        char*    _out,
                                        size_t   _capacity,
                                        size_t*  _out_size);


//==============================================================================
// 5.  IDENTITIES
//==============================================================================
// Whether a certificate names the host the client meant to reach -- the check
// that makes a verified chain mean anything -- done once, here, by the rules
// of RFC 9525 (which replaces RFC 6125):
//     - DNS names compare case-insensitively, ignoring one trailing dot.
//     - A wildcard counts only as the entire left-most label, as in
//       "*.example.com". It matches exactly one non-empty label and needs
//       at least two labels after it, so "*.com" matches nothing. Partial
//       wildcards ("w*.example.com") and wildcards elsewhere never match.
//     - An IP address literal matches only an IP address entry, octet for
//       octet, and never a DNS name.
//     - The subject common name is not consulted.
// Engines hand the kernel the peer certificate's subjectAltName entries as
// d_ssl_peer_name values, so every engine enforces the same rules. Host names
// are ASCII: an internationalized name must be converted to A-labels first.


// 5.1    Presented identifiers
//------------------------------------------------------------------------------
// 5.1.1
// d_ssl_name_kind
//   enum: the kind of a subjectAltName entry the kernel understands.
enum d_ssl_name_kind
{
    D_SSL_NAME_DNS = 1,
    D_SSL_NAME_IP  = 2
};

// 5.1.2
// d_ssl_peer_name
//   struct: one subjectAltName entry of a peer certificate, borrowed. For
// D_SSL_NAME_DNS, value holds the dNSName's ASCII characters; for
// D_SSL_NAME_IP, the 4 or 16 octets of the iPAddress.
struct d_ssl_peer_name
{
    enum d_ssl_name_kind kind;
    struct d_pack_bytes  value;
};

// 5.2    Host names and addresses
//------------------------------------------------------------------------------
// d_ssl_hostname_is_valid tests a reference host name: 1 to 253 characters
// in dot-separated labels of 1 to 63 letters, digits, hyphens, and
// underscores, no label beginning or ending with a hyphen, and at most one
// trailing dot. Underscores, invalid in host names proper, are accepted
// because deployed names use them. d_ssl_host_is_ip is true when the text is
// an IPv4 or IPv6 address literal as d_ssl_ip_parse reads it.
bool d_ssl_hostname_is_valid(struct d_pack_text _host);
bool d_ssl_host_is_ip(struct d_pack_text _host);

/**
 * @brief Parses an IP address literal into its binary form.
 *
 * @note IPv4 is strict dotted-quad: four decimal fields of 0 to 255 without
 *       leading zeros, which some libraries read as octal. IPv6 follows RFC
 *       4291 section 2.2: eight groups of one to four hex digits, one "::"
 *       standing for at least one zero group, and an optional dotted-quad
 *       tail. Brackets and zone suffixes ("%eth0") are not accepted; strip
 *       them first.
 *
 * @param[in]  _text        the literal.
 * @param[out] _out         receives the address, 4 octets for IPv4 and 16
 *                          for IPv6; untouched on failure.
 * @param[out] _out_length  receives 4 or 16.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_MALFORMED if `_text` is not an
 *         address literal; or D_SSL_STATUS_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_ssl_status d_ssl_ip_parse(struct d_pack_text _text,
                                 uint8_t            _out[D_SSL_IP_MAX],
                                 size_t*            _out_length);

/**
 * @brief Derives the Server Name Indication a client sends for a host.
 *
 * @note RFC 6066 section 3: the name goes without its trailing dot, and
 *       address literals are not permitted, so for them there is no name to
 *       send.
 *
 * @param[in]  _host     the host the client is connecting to.
 * @param[out] _out_sni  receives the name to send, borrowed from `_host`.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_NOT_FOUND if `_host` is an address
 *         literal (send no SNI); D_SSL_STATUS_MALFORMED if it is not a valid
 *         host name; or D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments.
 */
enum d_ssl_status d_ssl_sni_from_host(struct d_pack_text  _host,
                                      struct d_pack_text* _out_sni);

// 5.3    Identity matching
//------------------------------------------------------------------------------
// Pure predicates, by the rules at the head of this section; a malformed
// input on either side never matches. d_ssl_hostname_match compares one
// presented DNS name (_pattern, possibly a wildcard) with the reference host.
// d_ssl_ip_match compares one presented iPAddress (4 or 16 octets) with a
// reference address literal. d_ssl_identity_match is true if any of _count
// presented names matches _host, choosing IP or DNS matching by what _host
// is; _names may be NULL when _count is 0.
bool d_ssl_hostname_match(struct d_pack_text _pattern,
                          struct d_pack_text _host);
bool d_ssl_ip_match(struct d_pack_bytes _presented,
                    struct d_pack_text  _host);
bool d_ssl_identity_match(const struct d_ssl_peer_name* _names,
                          size_t                        _count,
                          struct d_pack_text            _host);


//==============================================================================
// 6.  WIRE HELPERS
//==============================================================================
// Two pieces of TLS wire format worth handling outside any library. ALPN
// lists are built by applications, checked by clients, and chosen from by
// servers, and one library's own selection routine has been the source of a
// memory disclosure (CVE-2024-5535). Stream sniffing tells a TLS record from
// anything else by its first bytes, which turns a plaintext peer on a TLS
// port into D_SSL_STATUS_NOT_TLS instead of a library's "wrong version
// number", and lets a server share one port between TLS and plaintext.


// 6.1    Application-layer protocol negotiation
//------------------------------------------------------------------------------
// An ALPN list travels as its wire encoding (RFC 7301 section 3.1): each
// protocol name as one length octet and 1 to 255 octets, concatenated. It is
// a d_pack_bytes throughout; protocol names are d_pack_text, since the
// registered ones ("http/1.1", "h2", "imap", "pop3") are ASCII.
//   d_ssl_alpn_is_valid is true for a non-empty list of well-formed entries
// that exactly fills its span, at most D_SSL_ALPN_LIST_MAX octets.
// d_ssl_alpn_contains is true if a valid list holds _protocol exactly.
bool d_ssl_alpn_is_valid(struct d_pack_bytes _list);
bool d_ssl_alpn_contains(struct d_pack_bytes _list,
                         struct d_pack_text  _protocol);

/**
 * @brief Encodes protocol names as an ALPN wire list, in order of preference.
 *
 * @note Two-call protocol: with `_out` NULL and `_capacity` 0, only the
 *       encoded size is stored.
 *
 * @param[in]  _protocols  the protocol names, each 1 to 255 octets.
 * @param[in]  _count      how many; at least one.
 * @param[out] _out        the destination, or NULL to measure.
 * @param[in]  _capacity   the size of `_out` in bytes.
 * @param[out] _out_size   receives the encoded size.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_BUFFER_TOO_SMALL, with `*_out_size`
 *         set; D_SSL_STATUS_MALFORMED if a name is empty or too long, or the
 *         list would exceed D_SSL_ALPN_LIST_MAX; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments or a zero
 *         `_count`.
 */
enum d_ssl_status d_ssl_alpn_encode(const struct d_pack_text* _protocols,
                                    size_t                    _count,
                                    void*                     _out,
                                    size_t                    _capacity,
                                    size_t*                   _out_size);

/**
 * @brief Steps through an ALPN wire list, one protocol per call.
 *
 * @param[in]     _list          the wire list.
 * @param[in,out] _offset        the cursor, 0 to start; advanced past each
 *                               entry read.
 * @param[out]    _out_protocol  receives the next name, borrowed from
 *                               `_list`.
 * @return D_SSL_STATUS_OK with a name; D_SSL_STATUS_NOT_FOUND at the end of
 *         the list; D_SSL_STATUS_MALFORMED if the entry at `*_offset` is
 *         empty or overruns the list; or D_SSL_STATUS_INVALID_ARGUMENT for
 *         NULL arguments or an offset past the end.
 */
enum d_ssl_status d_ssl_alpn_next(struct d_pack_bytes _list,
                                  size_t*             _offset,
                                  struct d_pack_text* _out_protocol);

/**
 * @brief Chooses the protocol a server answers an ALPN offer with.
 *
 * @note The first of the server's protocols that the client also offered
 *       wins (RFC 7301 section 3.2). The result points into `_offered`,
 *       which is where library selection callbacks expect it.
 *
 * @param[in]  _preferred     the server's list, most preferred first.
 * @param[in]  _offered       the list from the client's ClientHello.
 * @param[out] _out_protocol  receives the choice, borrowed from `_offered`.
 * @return D_SSL_STATUS_OK with a choice; D_SSL_STATUS_NOT_FOUND if the lists
 *         share nothing, which a server answers with no_application_protocol;
 *         D_SSL_STATUS_MALFORMED if either list is invalid; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for a NULL `_out_protocol`.
 */
enum d_ssl_status d_ssl_alpn_select(struct d_pack_bytes _preferred,
                                    struct d_pack_bytes _offered,
                                    struct d_pack_text* _out_protocol);

// 6.2    Stream sniffing
//------------------------------------------------------------------------------
// 6.2.1
// d_ssl_sniff_result
//   enum: what the first bytes of a stream are.
//     NEED_MORE  every byte so far fits a TLS stream, but there are too few
//                to be sure; call again with more.
//     TLS        a TLS record header opening a stream: a handshake or an
//                alert record, version 3.x, with a plausible length.
//     SSL2       an SSL 2.0-format ClientHello, which old clients sent to
//                offer newer versions. Engines today refuse it; it is still
//                a TLS peer rather than a plaintext one.
//     NOT_TLS    the bytes cannot open a TLS stream.
enum d_ssl_sniff_result
{
    D_SSL_SNIFF_NEED_MORE = 0,
    D_SSL_SNIFF_TLS       = 1,
    D_SSL_SNIFF_SSL2      = 2,
    D_SSL_SNIFF_NOT_TLS   = 3
};

// 6.2.2
// d_ssl_record_header
//   struct: the decoded header of a TLS record.
struct d_ssl_record_header
{
    uint8_t  content_type;
    uint16_t version;
    uint16_t length;
};

// 6.3    Sniffing operations
//------------------------------------------------------------------------------
// d_ssl_sniff judges the first bytes a peer sent, deciding NOT_TLS from as
// little as one byte when that byte already rules TLS out. On TLS, and when
// _out_header is non-NULL, it stores the header of the opening record.
// Given an empty prefix it returns NEED_MORE; it never fails.
enum d_ssl_sniff_result d_ssl_sniff(struct d_pack_bytes         _prefix,
                                    struct d_ssl_record_header* _out_header);


//==============================================================================
// 7.  CERTIFICATES AND TRUST
//==============================================================================
// Library-independent certificate handling: PEM framing (RFC 7468) and its
// base64 body, SHA-256 fingerprints of DER certificates, and where the
// platform keeps its trusted authorities. Libraries that take only DER --
// the OS-provided ones among them -- get it from PEM here, and fingerprints
// are computed by the kernel from the DER an engine hands it, so pins and
// displayed fingerprints mean the same thing under every engine. The
// fingerprint is SHA-256 over the whole certificate, as `openssl x509
// -fingerprint -sha256` prints it.


// 7.1    PEM
//------------------------------------------------------------------------------
// 7.1.1
// d_ssl_pem_block
//   struct: one PEM block, borrowed from the text it was found in: its label
// ("CERTIFICATE", "PRIVATE KEY") and its base64 body, line breaks included.
struct d_ssl_pem_block
{
    struct d_pack_text label;
    struct d_pack_text body;
};

// 7.2    PEM operations
//------------------------------------------------------------------------------
/**
 * @brief Finds the next PEM block at or after a cursor.
 *
 * @note A block is a "-----BEGIN <label>-----" line and the matching
 *       "-----END <label>-----" line. Text between blocks, such as the
 *       explanatory lines some tools write, is skipped. Call until
 *       D_SSL_STATUS_NOT_FOUND.
 *
 * @param[in]     _text    the PEM text, such as a whole CA bundle.
 * @param[in,out] _offset  the cursor, 0 to start; advanced past each block,
 *                         or to the end of `_text` on MALFORMED.
 * @param[out]    _out     receives the block, borrowed from `_text`.
 * @return D_SSL_STATUS_OK with a block; D_SSL_STATUS_NOT_FOUND when no
 *         further BEGIN line exists; D_SSL_STATUS_MALFORMED for a BEGIN line
 *         without its END, as in a truncated file; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments or an offset past
 *         the end.
 */
enum d_ssl_status d_ssl_pem_next(struct d_pack_text      _text,
                                 size_t*                 _offset,
                                 struct d_ssl_pem_block* _out);

/**
 * @brief Decodes a PEM body from base64: to DER, for a certificate.
 *
 * @note Spaces, tabs, and line breaks are skipped; any other character
 *       outside the base64 alphabet is an error, so the header lines of
 *       legacy encrypted keys ("Proc-Type:") are refused rather than
 *       misread. Padding must be complete. Two-call protocol: with `_out`
 *       NULL and `_capacity` 0, only the decoded size is stored.
 *
 * @param[in]  _body      the base64 text, from d_ssl_pem_block.body.
 * @param[out] _out       the destination, or NULL to measure.
 * @param[in]  _capacity  the size of `_out` in bytes.
 * @param[out] _out_size  receives the decoded size.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_BUFFER_TOO_SMALL, with `*_out_size`
 *         set; D_SSL_STATUS_MALFORMED for invalid base64; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments.
 */
enum d_ssl_status d_ssl_pem_decode(struct d_pack_text _body,
                                   void*              _out,
                                   size_t             _capacity,
                                   size_t*            _out_size);

// 7.3    Fingerprints
//------------------------------------------------------------------------------
// d_ssl_fingerprint_equal compares two fingerprints in constant time, so a
// pin check leaks nothing through timing.
bool d_ssl_fingerprint_equal(const uint8_t _a[D_SSL_FINGERPRINT_LENGTH],
                             const uint8_t _b[D_SSL_FINGERPRINT_LENGTH]);

/**
 * @brief Computes a certificate's fingerprint: the SHA-256 digest of its DER.
 *
 * @param[in]  _der  the DER-encoded certificate.
 * @param[out] _out  receives the 32-octet digest.
 * @return D_SSL_STATUS_OK, or D_SSL_STATUS_INVALID_ARGUMENT if `_out` is NULL
 *         or `_der` has a NULL pointer with a nonzero size.
 */
enum d_ssl_status d_ssl_fingerprint_compute(
                      struct d_pack_bytes _der,
                      uint8_t             _out[D_SSL_FINGERPRINT_LENGTH]);

/**
 * @brief Writes a fingerprint as colon-joined uppercase hex, "AB:CD:...:EF".
 *
 * @param[in]  _fingerprint  the 32-octet digest.
 * @param[out] _out          receives the text and a NUL.
 * @return D_SSL_STATUS_OK, or D_SSL_STATUS_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_ssl_status d_ssl_fingerprint_format(
                      const uint8_t _fingerprint[D_SSL_FINGERPRINT_LENGTH],
                      char          _out[D_SSL_FINGERPRINT_TEXT_LENGTH + 1]);

/**
 * @brief Reads a fingerprint as users supply it.
 *
 * @note Accepts 64 hex digits of either case, contiguous or as 32 pairs
 *       joined by colons. Tool output such as "sha256 Fingerprint=AB:CD:..."
 *       is accepted too: when the text holds an '=', only what follows the
 *       last one is read. Surrounding blanks are ignored.
 *
 * @param[in]  _text  the fingerprint text.
 * @param[out] _out   receives the 32-octet digest; untouched on failure.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_MALFORMED if `_text` is not a SHA-256
 *         fingerprint; or D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments.
 */
enum d_ssl_status d_ssl_fingerprint_parse(
                      struct d_pack_text _text,
                      uint8_t            _out[D_SSL_FINGERPRINT_LENGTH]);

// 7.4    Platform trust stores
//------------------------------------------------------------------------------
// The CA bundle files and certificate directories env_ssl.h lists for this
// platform (D_ENV_SSL_CA_FILES, D_ENV_SSL_CA_DIRS), most likely first, for an
// engine whose library cannot find the platform store itself. They are
// candidates: the kernel opens nothing, and an engine uses the first that
// exists. Each returns a static, NULL-terminated array and stores its length
// in *_out_count when that is non-NULL; the array is empty on platforms with
// an OS store. The conventional overrides are the environment variables
// SSL_CERT_FILE and SSL_CERT_DIR, which an engine consults first.
const char* const* d_ssl_ca_file_candidates(size_t* _out_count);
const char* const* d_ssl_ca_dir_candidates(size_t* _out_count);


//==============================================================================
// 8.  CONFIGURATION
//==============================================================================
// Everything a context needs to know, in one plain record the caller fills
// and owns. Its pointers are borrowed only for the duration of
// d_ssl_context_init -- the engine loads files and copies text while creating
// its context -- so a configuration may live on the stack.
//   d_ssl_config_init gives the safe defaults: TLS 1.2 or newer, a client
// that verifies the server's chain and name against the system's trusted
// authorities, a server that asks nothing of its clients. Weakening any of
// that is an explicit assignment. d_ssl_config_validate enforces the rules
// that hold whatever engine runs the configuration, and d_ssl_config_plan
// reduces the record to the decisions a handshake acts on, so every engine
// reads the same answers from the same inputs.


// 8.1    Configuration records
//------------------------------------------------------------------------------
// 8.1.1
// d_ssl_keylog_fn
//   function pointer: receives one line of NSS key-log format per secret a
// session derives, for a packet analyzer to decrypt a capture with. The line
// is NUL-terminated and has no newline. Only builds with D_CFG_SSL_KEYLOG
// honour it.
typedef void (*d_ssl_keylog_fn)(void*       _context,
                                const char* _line);

// 8.1.2
// d_ssl_config
//   struct: a TLS configuration. By group:
//     role, versions     the end this is, and the version window; a maximum
//                        of NONE means the newest the engine supports.
//     verification       the mode, and whether a client also checks the
//                        server's name (the reference host comes from each
//                        session, not from here).
//     trust anchors      the platform store, a PEM bundle file, an OpenSSL
//                        hashed directory, in-memory PEM, or any mix. A
//                        configuration that verifies needs at least one,
//                        unless it trusts pins alone.
//     local identity     a certificate chain (leaf first) and its private
//                        key, each from a file or from memory. Servers need
//                        one; a client supplies one for client
//                        authentication. The key is secret: the kernel never
//                        copies it, and the caller should wipe memory copies
//                        once the context exists.
//     negotiation        cipher strings in the engine's own syntax (NULL for
//                        its defaults) -- cipher_list for TLS 1.2 and older,
//                        cipher_suites for TLS 1.3 -- and an ALPN wire list,
//                        which a client offers and a server chooses from.
//                        alpn_required fails a handshake that settles on no
//                        protocol.
//     pinning            SHA-256 fingerprints of acceptable peer
//                        certificates. With pin_only false they add a check
//                        to chain verification; with pin_only true they
//                        replace it, which is how a self-signed server is
//                        trusted.
//     diagnostics        the key-log callback and its context.
struct d_ssl_config
{
    enum d_ssl_role        role;
    enum d_ssl_version     min_version;
    enum d_ssl_version     max_version;

    enum d_ssl_verify_mode verify;
    bool                   verify_hostname;

    bool                   use_system_trust;
    const char*            ca_file;
    const char*            ca_path;
    struct d_pack_text     ca_pem;

    const char*            certificate_file;
    const char*            private_key_file;
    struct d_pack_text     certificate_pem;
    struct d_pack_text     private_key_pem;

    const char*            cipher_list;
    const char*            cipher_suites;
    struct d_pack_bytes    alpn;
    bool                   alpn_required;

    size_t                 pin_count;
    uint8_t                pins[D_SSL_PINS_MAX][D_SSL_FINGERPRINT_LENGTH];
    bool                   pin_only;

    d_ssl_keylog_fn        keylog;
    void*                  keylog_context;
};

// 8.1.3
// d_ssl_plan
//   struct: the handshake decisions a configuration implies, resolved once.
// Engines act on these rather than re-deriving them from the configuration:
//     max_version          never NONE; D_SSL_VERSION_NEWEST unless capped,
//                          and lowered by d_ssl_context_init to what the
//                          engine supports.
//     verify_chain         validate the peer's chain against the trust
//                          anchors, failing the handshake if it does not
//                          validate. False for NONE, and for pin_only.
//     request_certificate  server: ask the client for a certificate.
//     require_certificate  the peer must present a certificate.
//     verify_hostname      client: the peer must be named for the host.
//     check_pins           the peer certificate must match a pin.
//     offer_alpn           an ALPN list was configured: a client offers it, a
//                          server selects from it.
//     require_alpn         a handshake without an agreed protocol fails.
struct d_ssl_plan
{
    enum d_ssl_role    role;
    enum d_ssl_version min_version;
    enum d_ssl_version max_version;
    bool               verify_chain;
    bool               request_certificate;
    bool               require_certificate;
    bool               verify_hostname;
    bool               check_pins;
    bool               offer_alpn;
    bool               require_alpn;
};

// 8.2    Configuration operations
//------------------------------------------------------------------------------
// d_ssl_config_init fills _config with the defaults for _role, clearing
// everything else; a NULL _config is ignored. The defaults are: versions
// D_SSL_VERSION_FLOOR through NONE (newest); for a client, verify REQUIRED
// with verify_hostname; for a server, verify NONE; use_system_trust true.
void d_ssl_config_init(struct d_ssl_config* _config,
                       enum d_ssl_role      _role);

/**
 * @brief Appends a fingerprint to a configuration's pins.
 *
 * @param[in,out] _config       the configuration.
 * @param[in]     _fingerprint  a 32-octet SHA-256 certificate fingerprint.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_BUFFER_TOO_SMALL if D_SSL_PINS_MAX
 *         pins are already set; or D_SSL_STATUS_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_ssl_status d_ssl_config_add_pin(
    struct d_ssl_config* _config,
    const uint8_t        _fingerprint[D_SSL_FINGERPRINT_LENGTH]);

/**
 * @brief Checks a configuration against the rules every engine shares.
 *
 * @note The rules:
 *       - the role is CLIENT or SERVER;
 *       - the minimum is TLS 1.0 through 1.3, and the maximum NONE or no
 *         lower; SSL 3.0 anywhere, or a minimum below TLS 1.2 in a build
 *         without D_CFG_SSL_ALLOW_LEGACY, is UNSUPPORTED;
 *       - OPTIONAL verification is for servers only;
 *       - a configuration that verifies, other than by pins alone, names a
 *         trust anchor;
 *       - a certificate and its key come together, each from a file or from
 *         memory but not both, and a server has them;
 *       - an ALPN list, if any, is valid, and alpn_required has one;
 *       - pins fit, pin_only has at least one, and pins are not set on a
 *         configuration that does not verify;
 *       - a key-log callback needs a D_CFG_SSL_KEYLOG build;
 *       - paths are non-empty, and cipher strings are printable ASCII.
 *
 * @param[in] _config  the configuration.
 * @return D_SSL_STATUS_OK if valid; D_SSL_STATUS_UNSUPPORTED for a request
 *         this build refuses; D_SSL_STATUS_INVALID_CONFIG for an
 *         inconsistent one; or D_SSL_STATUS_INVALID_ARGUMENT for a NULL
 *         `_config`, or a span in it with a NULL pointer and a nonzero size.
 */
enum d_ssl_status d_ssl_config_validate(const struct d_ssl_config* _config);

/**
 * @brief Validates a configuration and resolves it into a d_ssl_plan.
 *
 * @param[in]  _config    the configuration.
 * @param[out] _out_plan  receives the plan; untouched on failure.
 * @return D_SSL_STATUS_OK; a status of d_ssl_config_validate if the
 *         configuration is invalid; or D_SSL_STATUS_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_ssl_status d_ssl_config_plan(const struct d_ssl_config* _config,
                                    struct d_ssl_plan*         _out_plan);


//==============================================================================
// 9.  TRANSPORT
//==============================================================================
// How ciphertext reaches the peer: two callbacks and a context, the same
// shape the other net kernels use. A socket, a pipe, a proxy tunnel, or
// another TLS session -- d_ssl_session_transport presents a session as one
// -- can all carry a session, and the session never learns which.
//   A transport may already have carried plaintext. TLS can begin at any
// point in a stream, which is what STARTTLS-style upgrades need; the caller
// must ensure no bytes the peer sent before the upgrade are still buffered on
// its side, or they would be mistaken for the first bytes of the handshake.


// 9.1    The transport interface
//------------------------------------------------------------------------------
// 9.1.1
// d_ssl_transport_read_fn
//   function pointer: reads up to _capacity (at least 1) bytes into _buffer,
// storing the count in *_out_read. Returns D_SSL_STATUS_OK with a count of at
// least 1; D_SSL_STATUS_CONNECTION_CLOSED at the orderly end of the stream;
// D_SSL_STATUS_WOULD_BLOCK when a non-blocking transport has nothing now; or
// D_SSL_STATUS_TRANSPORT_ERROR.
typedef enum d_ssl_status (*d_ssl_transport_read_fn)(void*   _context,
                                                     void*   _buffer,
                                                     size_t  _capacity,
                                                     size_t* _out_read);

// 9.1.2
// d_ssl_transport_write_fn
//   function pointer: writes up to _size (at least 1) bytes from _data,
// storing the count taken in *_out_written. Returns D_SSL_STATUS_OK with a
// count of at least 1, or the statuses of d_ssl_transport_read_fn, where
// CONNECTION_CLOSED means the peer no longer reads.
typedef enum d_ssl_status (*d_ssl_transport_write_fn)(void*       _context,
                                                      const void* _data,
                                                      size_t      _size,
                                                      size_t*     _out_written);

// 9.1.3
// d_ssl_transport
//   struct: a byte stream a session runs over. Passed by value; holds no
// ownership of context.
struct d_ssl_transport
{
    d_ssl_transport_read_fn  read;
    d_ssl_transport_write_fn write;
    void*                    context;
};

// 9.2    The in-memory pipe
//------------------------------------------------------------------------------
// Two byte rings joined back to back: what end A writes, end B reads, and
// the reverse. Both ends run in one thread, so a client and a server session
// can handshake with each other with no socket at all -- the test bed for
// engines, for protocols carried over TLS, and for their resumption paths.
// Rings are bounded by the storage the caller gives them, and a full ring
// answers writes with WOULD_BLOCK, an empty one reads with WOULD_BLOCK, so
// every blocking path gets exercised. A per-end chunk limit forces short
// reads and writes; closing an end's writing side delivers
// CONNECTION_CLOSED to the other end once it has drained what was sent.

// 9.2.1
// d_ssl_ring
//   struct: a bounded byte ring over caller storage.
struct d_ssl_ring
{
    unsigned char* data;
    size_t         capacity;
    size_t         head;
    size_t         size;
    bool           closed;
};

// 9.2.2
// d_ssl_pipe_end
//   struct: one end of a pipe: the ring it reads, the ring it writes, and its
// chunk limit (0 for none).
struct d_ssl_pipe_end
{
    struct d_ssl_ring* inbound;
    struct d_ssl_ring* outbound;
    size_t             chunk;
};

// 9.2.3
// d_ssl_pipe
//   struct: a two-ended in-memory byte stream. Its ends point into the pipe
// itself, so an initialized pipe must not be copied or moved.
struct d_ssl_pipe
{
    struct d_ssl_ring     rings[2];
    struct d_ssl_pipe_end ends[2];
};

// 9.2.4
// d_ssl_pipe_side
//   enum: names an end of a pipe.
enum d_ssl_pipe_side
{
    D_SSL_PIPE_A = 0,
    D_SSL_PIPE_B = 1
};

// 9.3    Transport operations
//------------------------------------------------------------------------------
// Pipe accessors. d_ssl_pipe_transport returns the transport of end _side.
// d_ssl_pipe_set_chunk caps each read and write by that end at _chunk bytes
// (0 removes the cap). d_ssl_pipe_close marks that end's writing side
// finished: the other end reads CONNECTION_CLOSED after draining, and
// further writes by _side fail with TRANSPORT_ERROR. d_ssl_pipe_pending
// counts the bytes waiting for end _side to read. A NULL pipe or an unknown
// side is ignored, or yields an empty transport or zero.
struct d_ssl_transport d_ssl_pipe_transport(struct d_ssl_pipe*   _pipe,
                                            enum d_ssl_pipe_side _side);
void                   d_ssl_pipe_set_chunk(struct d_ssl_pipe*   _pipe,
                                            enum d_ssl_pipe_side _side,
                                            size_t               _chunk);
void                   d_ssl_pipe_close(struct d_ssl_pipe*   _pipe,
                                        enum d_ssl_pipe_side _side);
size_t                 d_ssl_pipe_pending(const struct d_ssl_pipe* _pipe,
                                          enum d_ssl_pipe_side     _side);

/**
 * @brief Initializes a pipe over caller storage.
 *
 * @param[out] _pipe        the pipe; it must not move once initialized.
 * @param[in]  _buffer_a    storage for the bytes end A sends; it must
 *                          outlive the pipe.
 * @param[in]  _capacity_a  its size in bytes; at least 1.
 * @param[in]  _buffer_b    storage for the bytes end B sends; it must
 *                          outlive the pipe.
 * @param[in]  _capacity_b  its size in bytes; at least 1.
 * @return D_SSL_STATUS_OK, or D_SSL_STATUS_INVALID_ARGUMENT for a NULL
 *         argument or a zero capacity.
 */
enum d_ssl_status d_ssl_pipe_init(struct d_ssl_pipe* _pipe,
                                  void*              _buffer_a,
                                  size_t             _capacity_a,
                                  void*              _buffer_b,
                                  size_t             _capacity_b);

/**
 * @brief Writes all of `_data` through a transport, looping over short writes.
 *
 * @param[in]  _transport    the transport.
 * @param[in]  _data         the bytes; may be NULL when `_size` is 0.
 * @param[in]  _size         how many.
 * @param[out] _out_written  receives how many were written, all of them on
 *                           success; may be NULL.
 * @return D_SSL_STATUS_OK once all are written; otherwise the transport's
 *         failure, with `*_out_written` telling how far it got, so that a
 *         WOULD_BLOCK caller resumes from there;
 *         D_SSL_STATUS_TRANSPORT_ERROR if the transport reports writing
 *         nothing or more than it was given; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for a transport without a write
 *         callback, or a NULL `_data` with a nonzero `_size`.
 */
enum d_ssl_status d_ssl_transport_write_all(
                      struct d_ssl_transport _transport,
                      const void*            _data,
                      size_t                 _size,
                      size_t*                _out_written);


//==============================================================================
// 10. ENGINES
//==============================================================================
// An engine binds the kernel to one TLS library. It is a table of functions
// -- struct d_ssl_engine -- that a derived module fills in and exports, and
// that an application hands to d_ssl_context_init. The kernel names no
// engine and links against no library; which engines exist is a matter of
// which derived modules a build compiles (cfg_ssl.h).
//   ENGINES ARE SANS-I/O. An engine never touches a transport: the kernel
// feeds it the ciphertext it reads, drains the ciphertext it must send, and
// drives it with handshake, read, write, and close steps. Every library can
// be run this way -- OpenSSL through memory BIOs, the others through their
// buffer or I/O-callback interfaces -- and in return the kernel implements
// once what would otherwise be written per library: blocking and
// non-blocking pumping, short reads and writes, WOULD_BLOCK resumption,
// truncation detection, and the identity, pin, and ALPN checks.
//   The contract, beyond each operation's own:
//     - Every operation of one session is called from one thread at a time.
//       Contexts may be shared across threads only if the engine says so.
//     - A step returns OK, WANT_READ (feed me more ciphertext, then call
//       again), or a failure. Engines never return WOULD_BLOCK or
//       TRANSPORT_ERROR; the kernel treats either as BACKEND_ERROR.
//     - Output is never refused: whatever a step produces is buffered inside
//       the engine until drained. The kernel drains before every step.
//     - On failure, an engine queues the alert it sends, if any, for the
//       kernel to deliver, and reports it through describe.
//     - An engine enforces the plan it was given at context creation --
//       versions, chain verification, certificate requests, ALPN -- inside
//       the handshake, where a failure can alert the peer. The kernel checks
//       the outcome again afterwards, and checks names and pins itself.


// 10.1   Engine vocabulary
//------------------------------------------------------------------------------
// 10.1.1
// d_ssl_engine_feature
//   enum: bits of d_ssl_engine.features, declaring what an engine can do.
// d_ssl_context_init refuses, with UNSUPPORTED, a configuration needing a
// feature its engine lacks.
//     TLS1_3        can negotiate TLS 1.3; without it, maxima are capped at
//                   TLS 1.2.
//     ALPN          can offer and select ALPN protocols.
//     SYSTEM_TRUST  can load the platform's trust store.
//     MEMORY_PEM    can load certificates, keys, and CAs from memory.
//     CLIENT_AUTH   can present and request client certificates.
//     KEYLOG        can report key-log lines.
//     HOSTNAME      checks the peer's name itself during the handshake. An
//                   engine without peer_name must have it; one with both is
//                   checked by the kernel as well.
//     LEGACY        can negotiate TLS 1.0 and 1.1.
enum d_ssl_engine_feature
{
    D_SSL_ENGINE_TLS1_3       = 0x0001,
    D_SSL_ENGINE_ALPN         = 0x0002,
    D_SSL_ENGINE_SYSTEM_TRUST = 0x0004,
    D_SSL_ENGINE_MEMORY_PEM   = 0x0008,
    D_SSL_ENGINE_CLIENT_AUTH  = 0x0010,
    D_SSL_ENGINE_KEYLOG       = 0x0020,
    D_SSL_ENGINE_HOSTNAME     = 0x0040,
    D_SSL_ENGINE_LEGACY       = 0x0080
};

// 10.1.2
// d_ssl_info
//   struct: what a session negotiated and learned, as far as it got:
//     version         the negotiated version, NONE until known.
//     cipher_id       the suite's IANA code point, 0 if unknown.
//     cipher          the suite's name, IANA spelling where the engine knows
//                     it, NUL-terminated; empty if unknown.
//     alpn            the agreed ALPN protocol, alpn_length octets, also
//                     NUL-terminated; empty if none.
//     verify          d_ssl_verify_flag bits; see section 4.
//     alert_sent      the last alert this end sent, or D_SSL_ALERT_NONE.
//     alert_received  the last alert the peer sent, or D_SSL_ALERT_NONE.
//     resumed         the handshake resumed an earlier session.
//     fingerprint     the SHA-256 fingerprint of the peer's certificate,
//                     valid when has_peer_certificate is true.
// Engines fill everything but has_peer_certificate and fingerprint, which
// the kernel computes, and the kernel's own verify bits, which it adds.
struct d_ssl_info
{
    enum d_ssl_version version;
    uint16_t           cipher_id;
    char               cipher[D_SSL_CIPHER_NAME_MAX];
    size_t             alpn_length;
    char               alpn[D_SSL_ALPN_PROTOCOL_MAX + 1];
    uint32_t           verify;
    int32_t            alert_sent;
    int32_t            alert_received;
    bool               resumed;
    bool               has_peer_certificate;
    uint8_t            fingerprint[D_SSL_FINGERPRINT_LENGTH];
};

// 10.2   Engine operations
//------------------------------------------------------------------------------
// 10.2.1
// d_ssl_engine_context_create_fn
//   function pointer: creates the engine's context from a validated
// configuration and its plan, loading trust anchors, the local identity,
// cipher settings, and the ALPN list. It must copy whatever it keeps, since
// neither argument outlives the call. Stores the context in *_out_context and
// returns OK, or returns UNSUPPORTED, INVALID_CONFIG (a file that does not
// load, a key that does not match its certificate, a cipher string the
// library rejects), OUT_OF_MEMORY, or BACKEND_ERROR.
typedef enum d_ssl_status (*d_ssl_engine_context_create_fn)(
    const struct d_ssl_config* _config,
    const struct d_ssl_plan*   _plan,
    void**                     _out_context);

// 10.2.2
// d_ssl_engine_context_destroy_fn
//   function pointer: frees a context. Every session made from it has been
// destroyed first.
typedef void (*d_ssl_engine_context_destroy_fn)(void* _context);

// 10.2.3
// d_ssl_engine_session_create_fn
//   function pointer: creates a session in a context. For a client, _host is
// the reference identity to check the peer against, or NULL when the plan
// does not verify host names, and _sni the server name to send, or NULL for
// none; servers receive NULL for both. Both are NUL-terminated and must be
// copied. Stores the session in *_out_session and returns OK, OUT_OF_MEMORY,
// or BACKEND_ERROR.
typedef enum d_ssl_status (*d_ssl_engine_session_create_fn)(
    void*       _context,
    const char* _host,
    const char* _sni,
    void**      _out_session);

// 10.2.4
// d_ssl_engine_session_destroy_fn
//   function pointer: frees a session, in whatever state it was left.
typedef void (*d_ssl_engine_session_destroy_fn)(void* _session);

// 10.2.5
// d_ssl_engine_feed_fn
//   function pointer: hands the engine ciphertext read from the peer. It must
// take all _size bytes (at least 1), buffering what its next step does not
// consume. Returns OK, OUT_OF_MEMORY, or BACKEND_ERROR.
typedef enum d_ssl_status (*d_ssl_engine_feed_fn)(void*       _session,
                                                  const void* _data,
                                                  size_t      _size);

// 10.2.6
// d_ssl_engine_drain_fn
//   function pointer: moves up to _capacity bytes of pending outbound
// ciphertext into _buffer, removing them from the engine, and stores the
// count in *_out_size -- 0 when nothing is pending. Returns OK or
// BACKEND_ERROR.
typedef enum d_ssl_status (*d_ssl_engine_drain_fn)(void*   _session,
                                                   void*   _buffer,
                                                   size_t  _capacity,
                                                   size_t* _out_size);

// 10.2.7
// d_ssl_engine_step_fn
//   function pointer: advances the handshake, or queues close_notify. As
// handshake: returns OK once the handshake is complete (and on every call
// after), WANT_READ while it awaits the peer, or a failure. As close: queues
// close_notify and returns OK; calling it twice is harmless.
typedef enum d_ssl_status (*d_ssl_engine_step_fn)(void* _session);

// 10.2.8
// d_ssl_engine_read_fn
//   function pointer: decrypts up to _capacity (at least 1) bytes of
// application data into _buffer. Returns OK with *_out_read at least 1,
// WANT_READ when no complete record is buffered, CONNECTION_CLOSED once the
// peer's close_notify arrives, or a failure. Handshake messages that arrive
// after the handshake (session tickets, key updates) are absorbed here.
typedef enum d_ssl_status (*d_ssl_engine_read_fn)(void*   _session,
                                                  void*   _buffer,
                                                  size_t  _capacity,
                                                  size_t* _out_read);

// 10.2.9
// d_ssl_engine_write_fn
//   function pointer: encrypts application data. Returns OK having taken at
// least 1 of _size (at least 1) bytes, storing the count in *_out_written;
// WANT_READ if the engine must hear from the peer first; or a failure.
typedef enum d_ssl_status (*d_ssl_engine_write_fn)(void*       _session,
                                                   const void* _data,
                                                   size_t      _size,
                                                   size_t*     _out_written);

// 10.2.10
// d_ssl_engine_describe_fn
//   function pointer: fills the engine's part of a d_ssl_info (see 10.1.2)
// with what the session knows so far. The kernel resets the record first --
// version NONE, no alerts, verify NOT_PERFORMED -- so fields the engine
// cannot tell may be left alone. The verify bits are the chain result alone:
// NONE if the chain validated, NOT_PERFORMED if it was not checked.
typedef void (*d_ssl_engine_describe_fn)(void*              _session,
                                         struct d_ssl_info* _out);

// 10.2.11
// d_ssl_engine_peer_name_fn
//   function pointer: stores the _index-th DNS or IP subjectAltName entry of
// the peer's certificate in *_out, borrowed until the next call into the
// engine, and returns true; returns false past the last entry, or when no
// certificate was presented. Entries of other kinds are skipped.
typedef bool (*d_ssl_engine_peer_name_fn)(void*                   _session,
                                          size_t                  _index,
                                          struct d_ssl_peer_name* _out);

// 10.2.12
// d_ssl_engine_peer_certificate_fn
//   function pointer: stores the DER encoding of the peer's certificate in
// *_out_der, borrowed until the next call into the engine, and returns true;
// returns false when the peer presented none.
typedef bool (*d_ssl_engine_peer_certificate_fn)(
    void*                _session,
    struct d_pack_bytes* _out_der);

// 10.3   The engine interface
//------------------------------------------------------------------------------
// 10.3.1
// d_ssl_engine
//   struct: an engine: its name, its library, its features, and its
// operations. Engines are immutable static data. Every operation is required
// except peer_name, which an engine with D_SSL_ENGINE_HOSTNAME may omit, and
// peer_certificate, without which pins cannot be checked and fingerprints
// stay unknown.
struct d_ssl_engine
{
    const char*                      name;
    enum d_ssl_backend               backend;
    uint32_t                         features;
    d_ssl_engine_context_create_fn   context_create;
    d_ssl_engine_context_destroy_fn  context_destroy;
    d_ssl_engine_session_create_fn   session_create;
    d_ssl_engine_session_destroy_fn  session_destroy;
    d_ssl_engine_feed_fn             feed;
    d_ssl_engine_drain_fn            drain;
    d_ssl_engine_step_fn             handshake;
    d_ssl_engine_read_fn             read;
    d_ssl_engine_write_fn            write;
    d_ssl_engine_step_fn             close;
    d_ssl_engine_describe_fn         describe;
    d_ssl_engine_peer_name_fn        peer_name;
    d_ssl_engine_peer_certificate_fn peer_certificate;
};


//==============================================================================
// 11. CONTEXTS AND SESSIONS
//==============================================================================
// A context is a configuration made live in an engine: trust loaded, keys
// parsed, the plan fixed. A session is one TLS connection over one
// transport, made from a context; many sessions share a context. The kernel
// owns the session's pump -- moving ciphertext between the engine and the
// transport -- and its policy checks, so an application sees the same
// behaviour under every engine:
//     - Operations on a blocking transport complete or fail. On a
//       non-blocking one, any may return WOULD_BLOCK; retry the same call,
//       with the same arguments, once d_ssl_session_wants says the transport
//       is ready.
//     - Reads and writes perform the handshake first if it is not done.
//     - A handshake is complete only once its last flight is on the wire,
//       and only after the kernel has checked the version, ALPN, the peer
//       certificate, pins, and the host name. A failed check discards that
//       last flight unsent.
//     - The first bytes from the peer are sniffed; a peer not speaking TLS
//       fails with NOT_TLS before the engine sees anything.
//     - A transport that ends before close_notify fails with UNEXPECTED_EOF,
//       not CONNECTION_CLOSED, so truncation cannot pass for a clean end.
//       Protocols that frame their own messages (POP3 after QUIT, say) may
//       treat it as benign.
//     - Failures are sticky: once a session fails, every later operation
//       returns the same status.
// Neither contexts nor sessions allocate in the kernel; engines allocate.
// Initialized contexts and sessions must not be copied.


// 11.1   Contexts
//------------------------------------------------------------------------------
// 11.1.1
// d_ssl_context
//   struct: a live configuration: the engine, its context handle, the plan,
// and the pins the kernel checks. Read-only once initialized.
struct d_ssl_context
{
    const struct d_ssl_engine* engine;
    void*                      handle;
    struct d_ssl_plan          plan;
    size_t                     pin_count;
    uint8_t                    pins[D_SSL_PINS_MAX][D_SSL_FINGERPRINT_LENGTH];
};

// 11.2   Context operations
//------------------------------------------------------------------------------
/**
 * @brief Makes a configuration live in an engine.
 *
 * @note Validates the configuration, resolves its plan, caps the maximum
 *       version at what the engine supports, refuses what the engine's
 *       features cannot honour, and has the engine create its context,
 *       which loads every file and PEM text the configuration names.
 *
 * @param[out] _context  the context to initialize.
 * @param[in]  _engine   the engine, with every required operation.
 * @param[in]  _config   the configuration; borrowed for this call only.
 * @post   On success the context owns the engine's context; release it with
 *         d_ssl_context_destroy, after every session made from it. On
 *         failure the context is left empty and need not be destroyed.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_UNSUPPORTED if the build or the
 *         engine cannot honour the configuration; a status of
 *         d_ssl_config_validate; the engine's failure from creating its
 *         context; or D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments or an
 *         engine missing a required operation.
 */
enum d_ssl_status d_ssl_context_init(struct d_ssl_context*      _context,
                                     const struct d_ssl_engine* _engine,
                                     const struct d_ssl_config* _config);

/**
 * @brief Releases a context's engine context and clears the record.
 *
 * @param[in,out] _context  the context; NULL is ignored.
 * @pre    Every session made from the context has been destroyed.
 * @post   The context is empty; destroying it again is harmless.
 */
void d_ssl_context_destroy(struct d_ssl_context* _context);

// 11.3   Sessions
//------------------------------------------------------------------------------
// 11.3.1
// d_ssl_state
//   enum: where a session is.
//     IDLE         initialized; no bytes exchanged.
//     HANDSHAKING  the handshake is under way.
//     ESTABLISHED  application data may flow. A shutdown in progress, or a
//                  close_notify received, is tracked alongside.
//     CLOSED       both ends have said close_notify, or the session was
//                  shut down before it was established.
//     FAILED       a failure ended the session; see d_ssl_session.failure.
enum d_ssl_state
{
    D_SSL_STATE_IDLE        = 0,
    D_SSL_STATE_HANDSHAKING = 1,
    D_SSL_STATE_ESTABLISHED = 2,
    D_SSL_STATE_CLOSED      = 3,
    D_SSL_STATE_FAILED      = 4
};

// 11.3.2
// d_ssl_want
//   enum: what a session waits on after returning WOULD_BLOCK: the transport
// becoming readable, or writable. WRITE also follows an OK write whose
// ciphertext could not all be sent; see d_ssl_session_write.
enum d_ssl_want
{
    D_SSL_WANT_NOTHING = 0,
    D_SSL_WANT_READ    = 1,
    D_SSL_WANT_WRITE   = 2
};

// 11.3.3
// d_ssl_session
//   struct: one TLS connection. The fields are the kernel's; read them
// through the accessors below. stage holds ciphertext drained from the
// engine but not yet accepted by the transport. engine_done marks an engine
// handshake that has finished, and checked the kernel's checks passing,
// while the last flight may still be pending. sniff collects the peer's
// first bytes until they are judged. bytes_in and bytes_out count the
// ciphertext moved through the transport.
struct d_ssl_session
{
    const struct d_ssl_context* context;
    void*                       handle;
    struct d_ssl_transport      transport;
    enum d_ssl_state            state;
    enum d_ssl_status           failure;
    enum d_ssl_want             want;
    bool                        engine_done;
    bool                        checked;
    bool                        sniffed;
    bool                        sent_close;
    bool                        received_close;
    bool                        host_is_ip;
    size_t                      sniff_length;
    uint8_t                     sniff[D_SSL_RECORD_HEADER_SIZE];
    char                        host[D_SSL_HOSTNAME_MAX + 2];
    struct d_ssl_info           info;
    uint64_t                    bytes_in;
    uint64_t                    bytes_out;
    size_t                      stage_start;
    size_t                      stage_end;
    unsigned char               stage[D_SSL_IO_CAPACITY];
};

// 11.4   Session operations
//------------------------------------------------------------------------------
/**
 * @brief Creates a session over a transport.
 *
 * @note A client names the host it is connecting to: a host name or an IP
 *       address literal. It becomes the reference identity the certificate
 *       is checked against and, for a name, the SNI sent. One trailing dot
 *       is dropped. A server passes an empty `_host`.
 *
 * @param[out] _session    the session to initialize.
 * @param[in]  _context    an initialized context; it must outlive the
 *                         session.
 * @param[in]  _transport  the transport, with both callbacks.
 * @param[in]  _host       client: the host, empty only if the plan does not
 *                         verify host names. Server: empty.
 * @post   On success the session owns the engine's session; release it with
 *         d_ssl_session_destroy. On failure it holds nothing, and destroying
 *         it is harmless.
 * @return D_SSL_STATUS_OK; D_SSL_STATUS_MALFORMED if `_host` is neither a
 *         valid host name nor an address literal;
 *         D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments, an empty
 *         context, a transport missing a callback, a client that verifies
 *         names given no host, or a server given one; or the engine's
 *         failure from creating its session.
 */
enum d_ssl_status d_ssl_session_init(struct d_ssl_session*       _session,
                                     const struct d_ssl_context* _context,
                                     struct d_ssl_transport      _transport,
                                     struct d_pack_text          _host);

/**
 * @brief Releases a session's engine session and clears the record.
 *
 * @note Nothing is sent: shut the session down first for an orderly close.
 *
 * @param[in,out] _session  the session; NULL is ignored.
 * @post   The session is empty; destroying it again is harmless. Transports
 *         made from it by d_ssl_session_transport must no longer be used.
 */
void d_ssl_session_destroy(struct d_ssl_session* _session);

/**
 * @brief Runs the handshake to completion.
 *
 * @note Flights are pumped both ways. Before the last one is sent, the result
 *       is checked against the plan: the version window, ALPN, certificate
 *       presence, pins, the host name, and the engine's chain verdict.
 *       Whatever the engine emits as it completes, such as a TLS 1.3
 *       server's session tickets, counts as the last flight. On an
 *       established session this only flushes.
 *
 * @param[in,out] _session  the session.
 * @return D_SSL_STATUS_OK once established; D_SSL_STATUS_WOULD_BLOCK, to be
 *         retried; D_SSL_STATUS_NOT_TLS, _HANDSHAKE_FAILED, _VERIFY_FAILED
 *         (d_ssl_session_info gives the flags), _PROTOCOL_ERROR, or
 *         _UNEXPECTED_EOF for a failed handshake; a transport or engine
 *         failure; D_SSL_STATUS_WRONG_STATE on a closed session; or the
 *         sticky failure of a failed one.
 */
enum d_ssl_status d_ssl_session_handshake(struct d_ssl_session* _session);

/**
 * @brief Reads application data, handshaking first if needed.
 *
 * @param[in,out] _session   the session.
 * @param[out]    _buffer    the destination.
 * @param[in]     _capacity  its size; at least 1.
 * @param[out]    _out_read  receives the count: at least 1 on success, 0
 *                           otherwise.
 * @return D_SSL_STATUS_OK with data; D_SSL_STATUS_CONNECTION_CLOSED once the
 *         peer has sent close_notify, and on every read after;
 *         D_SSL_STATUS_WOULD_BLOCK; D_SSL_STATUS_UNEXPECTED_EOF if the
 *         transport ended first; a handshake, protocol, transport, or engine
 *         failure; or D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments or a
 *         zero `_capacity`.
 */
enum d_ssl_status d_ssl_session_read(struct d_ssl_session* _session,
                                     void*                 _buffer,
                                     size_t                _capacity,
                                     size_t*               _out_read);

/**
 * @brief Encrypts and sends application data, handshaking first if needed.
 *
 * @note Ciphertext pending from earlier calls is flushed first, and new data
 *       is taken only once it has gone, which bounds what a session buffers.
 *       Data taken is committed: if the transport then blocks, the call
 *       still returns OK with the count, d_ssl_session_wants reports
 *       D_SSL_WANT_WRITE, and the rest leaves with the next operation or
 *       d_ssl_session_flush.
 *
 * @param[in,out] _session      the session.
 * @param[in]     _data         the bytes.
 * @param[in]     _size         how many; at least 1.
 * @param[out]    _out_written  receives how many were taken: at least 1 on
 *                              success, 0 otherwise.
 * @return D_SSL_STATUS_OK with a count; D_SSL_STATUS_WOULD_BLOCK before
 *         anything was taken; D_SSL_STATUS_WRONG_STATE after this end's
 *         close_notify; a handshake, transport, or engine failure; or
 *         D_SSL_STATUS_INVALID_ARGUMENT for NULL arguments or a zero
 *         `_size`.
 */
enum d_ssl_status d_ssl_session_write(struct d_ssl_session* _session,
                                      const void*           _data,
                                      size_t                _size,
                                      size_t*               _out_written);

/**
 * @brief Writes all of `_data` and flushes it.
 *
 * @note d_ssl_session_write in a loop, then d_ssl_session_flush. On a
 *       blocking transport it returns with everything on the wire.
 *
 * @param[in,out] _session      the session.
 * @param[in]     _data         the bytes; may be NULL when `_size` is 0.
 * @param[in]     _size         how many.
 * @param[out]    _out_written  receives how many were taken; may be NULL.
 * @return D_SSL_STATUS_OK once all are taken and sent;
 *         D_SSL_STATUS_WOULD_BLOCK, after which a caller resumes with the
 *         remainder, or with d_ssl_session_flush once `*_out_written`
 *         equals `_size`; or a failure of d_ssl_session_write.
 */
enum d_ssl_status d_ssl_session_write_all(struct d_ssl_session* _session,
                                          const void*           _data,
                                          size_t                _size,
                                          size_t*               _out_written);

/**
 * @brief Sends the ciphertext a session still holds.
 *
 * @note Staged bytes go first, then whatever the engine has pending. Only
 *       this session's ciphertext moves: when its transport is itself a
 *       session, that one is flushed separately.
 *
 * @param[in,out] _session  the session.
 * @return D_SSL_STATUS_OK once nothing is pending; D_SSL_STATUS_WOULD_BLOCK
 *         if the transport stopped taking bytes; D_SSL_STATUS_UNEXPECTED_EOF
 *         if the peer stopped reading; a transport or engine failure; or the
 *         sticky failure of a failed session.
 */
enum d_ssl_status d_ssl_session_flush(struct d_ssl_session* _session);

/**
 * @brief Closes this end of a session: queues close_notify and flushes it.
 *
 * @note It does not wait for the peer's close_notify, which RFC 8446 section
 *       6.1 does not require, but reads remain possible until it arrives;
 *       the session is CLOSED once both have been seen. A session not yet
 *       established is simply marked CLOSED, sending nothing.
 *
 * @param[in,out] _session  the session.
 * @return D_SSL_STATUS_OK once close_notify is on the wire, or the session
 *         was already closed; D_SSL_STATUS_WOULD_BLOCK, to be retried; a
 *         transport or engine failure; or the sticky failure of a failed
 *         session.
 */
enum d_ssl_status d_ssl_session_shutdown(struct d_ssl_session* _session);

// Session accessors; they never fail. d_ssl_session_state and
// d_ssl_session_wants report where the session is and what it waits on (a
// NULL or destroyed session reads as FAILED and NOTHING). d_ssl_session_info
// copies what the session negotiated and learned into *_out, and returns
// false (leaving *_out alone) for NULL arguments or a destroyed session.
// d_ssl_session_transport presents the session as a transport over its
// plaintext, for a protocol to run over or another session to nest in; its
// reads and writes are d_ssl_session_read and d_ssl_session_write. Like them,
// a write through it may leave ciphertext pending in the session, so a caller
// streaming data one way through a stack flushes each session in it.
enum d_ssl_state d_ssl_session_state(const struct d_ssl_session* _session);
enum d_ssl_want  d_ssl_session_wants(const struct d_ssl_session* _session);
bool             d_ssl_session_info(const struct d_ssl_session* _session,
                                    struct d_ssl_info*          _out);

struct d_ssl_transport d_ssl_session_transport(struct d_ssl_session* _session);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSL_SSL_H
