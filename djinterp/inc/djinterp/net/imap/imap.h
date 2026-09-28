/*******************************************************************************
* djinterp [net]                                                          imap.h
*
* djinterp IMAP common module.
*   The protocol kernel every IMAP module builds on: the vocabulary, and the
* pure codecs that do not depend on how bytes reach a server. A backend (the
* native engine, libcurl, libetpan, Mailutils, VMime) adds a transport and a
* session; everything the backends would otherwise each restate lives here
* once:
*     - backend identity, mirroring env_imap.h                            [1]
*     - one error vocabulary for every backend, server, and transport     [2]
*     - ports and limits                                                  [3]
*     - connection states, commands, and command tags                     [4]
*     - transport security and SASL mechanism names                       [5]
*     - capability parsing                                                [6]
*     - connection options, their validation, and mechanism selection     [7]
*     - message flags, mailbox attributes, and STATUS items               [8]
*     - atom / quoted / literal encoding, and modified UTF-7 names        [9]
*     - sequence sets                                                     [10]
*     - IMAP dates, with Unix-time conversion                             [11]
*     - status responses: tags, conditions, and response codes            [12]
*     - a response lexer                                                  [13]
*     - LIST and SEARCH data, and string and mailbox-name decoding        [14]
*   Nothing here allocates, performs I/O, or touches a third-party header.
* Text arrives as struct d_pack_text and output leaves through struct
* d_pack_sink, both from sink_common.h: the framework's one borrowed span and
* one byte destination, reused rather than restated. Parsed results are views
* into the caller's input, and live exactly as long as it does.
*   Protocol basis: IMAP4rev1 (RFC 3501) and IMAP4rev2 (RFC 9051), plus the
* extensions each section names. Keywords compare case-insensitively in ASCII
* only; the C library's locale-sensitive case folding is deliberately unused.
*   Deliberately absent: SASL payload construction, which POP3 and SMTP share
* and which belongs in a SASL module; FETCH, ENVELOPE, and BODYSTRUCTURE
* parsing, which the lexer supports and a later module should own; and any
* transport.
*   It is compiled as C and exports C linkage, so C++ modules consume it
* directly.
*
*
* path:      /inc/djinterp/net/imap/imap.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BACKENDS
    --------
    1.  Backend identifiers
         1.  d_imap_backend
         2.  D_IMAP_BACKEND_COUNT
    2.  Backend queries
2.  RESULTS
    -------
    1.  Error codes
         1.  d_imap_error
         2.  D_IMAP_ERROR_COUNT
    2.  Error queries
3.  PROTOCOL CONSTANTS
    ------------------
    1.  Ports
         1.  D_IMAP_PORT
         2.  D_IMAP_PORT_TLS
    2.  Limits
         1.  D_IMAP_NUMBER_MAX
         2.  D_IMAP_NUMBER64_MAX
         3.  D_IMAP_COMMAND_LINE_LIMIT
         4.  D_IMAP_RESPONSE_LINE_LIMIT_DEFAULT
         5.  D_IMAP_LITERAL_LIMIT_DEFAULT
         6.  D_IMAP_LITERAL_MINUS_MAX
4.  SESSION
    -------
    1.  Connection states
         1.  d_imap_state
         2.  D_IMAP_STATE_COUNT
    2.  Commands
         1.  d_imap_command
         2.  D_IMAP_COMMAND_COUNT
    3.  State and command queries
    4.  Tags
         1.  D_IMAP_TAG_PREFIX_MAX
         2.  D_IMAP_TAG_CAPACITY
         3.  d_imap_tag
         4.  d_imap_tagger
    5.  Tag operations
5.  SECURITY AND AUTHENTICATION
    ---------------------------
    1.  Transport security
         1.  d_imap_security
         2.  D_IMAP_SECURITY_COUNT
    2.  Authentication mechanisms
         1.  d_imap_auth_mechanism
         2.  D_IMAP_AUTH_COUNT
         3.  D_IMAP_AUTH_BIT
         4.  D_IMAP_AUTH_DEFAULT_SET
    3.  Security and mechanism queries
6.  CAPABILITIES
    ------------
    1.  Capability vocabulary
         1.  d_imap_capability
         2.  D_IMAP_CAPABILITY_COUNT
         3.  D_IMAP_CAPABILITY_BIT
         4.  d_imap_capabilities
    2.  Capability operations
7.  OPTIONS
    -------
    1.  Option flags
         1.  D_IMAP_OPTION_NO_VERIFY_PEER
         2.  D_IMAP_OPTION_NO_VERIFY_HOST
         3.  D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH
    2.  Option block
         1.  d_imap_options
    3.  Option operations
8.  MESSAGE AND MAILBOX ATTRIBUTES
    ------------------------------
    1.  Message flags
         1.  d_imap_flag
         2.  D_IMAP_FLAG_COUNT
         3.  D_IMAP_FLAG_BIT
    2.  Mailbox attributes
         1.  d_imap_mailbox_attribute
         2.  D_IMAP_MAILBOX_ATTRIBUTE_COUNT
         3.  D_IMAP_MAILBOX_BIT
         4.  D_IMAP_MAILBOX_SPECIAL_USE
    3.  STATUS items
         1.  d_imap_status_item
         2.  D_IMAP_STATUS_ITEM_COUNT
         3.  D_IMAP_STATUS_BIT
         4.  d_imap_status_data
    4.  Attribute operations
9.  STRINGS AND MAILBOX NAMES
    -------------------------
    1.  String forms
         1.  d_imap_string_form
         2.  D_IMAP_STRING_UTF8
         3.  D_IMAP_STRING_LITERAL_PLUS
         4.  D_IMAP_STRING_LITERAL_MINUS
         5.  D_IMAP_STRING_LIST_WILDCARDS
    2.  String writers
    3.  Mailbox names
10. SEQUENCE SETS
    -------------
    1.  Ranges
         1.  D_IMAP_SEQ_STAR
         2.  d_imap_seq_range
    2.  Sequence-set operations
11. DATES
    -----
    1.  Broken-down time
         1.  d_imap_datetime
    2.  Date operations
12. RESPONSES
    ---------
    1.  Conditions
         1.  d_imap_condition
         2.  D_IMAP_CONDITION_COUNT
    2.  Response codes
         1.  d_imap_code
         2.  D_IMAP_CODE_COUNT
    3.  Condition and code queries
    4.  Status responses
         1.  d_imap_response_kind
         2.  d_imap_response
    5.  Response parsing
13. LEXER
    -----
    1.  Tokens
         1.  d_imap_token_kind
         2.  D_IMAP_TOKEN_ESCAPED
         3.  D_IMAP_TOKEN_NONSYNC
         4.  D_IMAP_TOKEN_BINARY
         5.  D_IMAP_LEX_ASTRING
         6.  d_imap_token
         7.  d_imap_lexer
    2.  Lexer operations
14. RESPONSE DATA
    -------------
    1.  String values
    2.  Mailbox listings
         1.  d_imap_list_entry
    3.  Search results
*/

#ifndef DJINTERP_NET_IMAP_IMAP_H
#define DJINTERP_NET_IMAP_IMAP_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
#include <stdint.h>                    // int32_t, uint32_t, int64_t, uint64_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_sink, d_pack_text


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C object. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  BACKENDS
//==============================================================================
// Every implementation an IMAP module can sit on. The values are env_imap.h's
// D_ENV_IMAP_BACKEND_* identifiers; imap.c asserts the identity, so that this
// header need not include the detection layer to state it.


// 1.1    Backend identifiers
//------------------------------------------------------------------------------
// 1.1.1
// d_imap_backend
//   enum: an IMAP implementation. Converting to or from a
// D_ENV_IMAP_BACKEND_* identifier is the identity.
enum d_imap_backend
{
    D_IMAP_BACKEND_NONE      = 0,
    D_IMAP_BACKEND_NATIVE    = 1,  // the framework's engine over net transport
    D_IMAP_BACKEND_CURL      = 2,
    D_IMAP_BACKEND_LIBETPAN  = 3,
    D_IMAP_BACKEND_MAILUTILS = 4,
    D_IMAP_BACKEND_VMIME     = 5   // C++ only
};

// 1.1.2
// D_IMAP_BACKEND_COUNT
//   constant: the number of enumerators in d_imap_backend.
#define D_IMAP_BACKEND_COUNT 6

// 1.2    Backend queries
//------------------------------------------------------------------------------
// backend queries -- none allocate or fail. Names match env_imap.h's
// D_ENV_IMAP_BACKEND_NAME spellings, and an unknown value reads "unknown".
// Availability describes the environment imap.c was compiled in, which is C,
// so VMime never reads as available here; C++ code tests
// D_ENV_IMAP_VMIME_AVAILABLE directly. The default is D_ENV_IMAP_BACKEND.
const char*         d_imap_backend_name(enum d_imap_backend _backend);
bool                d_imap_backend_is_available(enum d_imap_backend _backend);
enum d_imap_backend d_imap_backend_default(void);


//==============================================================================
// 2.  RESULTS
//==============================================================================
// One error vocabulary for every IMAP module. This module produces only the
// first group; the others exist so that a caller sees the same codes whichever
// backend it runs on. Every fallible function returns enum d_imap_error, and
// D_IMAP_ERROR_NONE is 0, so `if (error)` tests for failure.


// 2.1    Error codes
//------------------------------------------------------------------------------
// 2.1.1
// d_imap_error
//   enum: the outcome of an IMAP operation. Values are pinned and
// append-only, since backends and callers switch on them.
enum d_imap_error
{
    D_IMAP_ERROR_NONE             = 0,   // success
    // produced by this module, and by every backend
    D_IMAP_ERROR_INVALID_ARGUMENT = 1,   // NULL pointer, bad enum, bad option
    D_IMAP_ERROR_SINK             = 2,   // the output sink refused bytes
    D_IMAP_ERROR_CAPACITY         = 3,   // a caller's array is too small
    D_IMAP_ERROR_SYNTAX           = 4,   // input is not valid IMAP syntax
    D_IMAP_ERROR_INCOMPLETE       = 5,   // input ends mid-item; read more
    D_IMAP_ERROR_RANGE            = 6,   // a number or field is out of range
    D_IMAP_ERROR_ENCODING         = 7,   // invalid UTF-8 or modified UTF-7
    D_IMAP_ERROR_LIMIT            = 8,   // a configured limit was exceeded
    // session outcomes, produced by backends
    D_IMAP_ERROR_STATE            = 9,   // command not valid in this state
    D_IMAP_ERROR_UNSUPPORTED      = 10,  // not in the build, backend, or server
    D_IMAP_ERROR_SECURITY         = 11,  // refused by the security policy
    D_IMAP_ERROR_AUTH             = 12,  // authentication failed
    // reported by the server
    D_IMAP_ERROR_NO               = 13,  // tagged NO
    D_IMAP_ERROR_BAD              = 14,  // tagged or untagged BAD
    D_IMAP_ERROR_BYE              = 15,  // BYE: the server ended the session
    D_IMAP_ERROR_PROTOCOL         = 16,  // the server broke the protocol
    // transport
    D_IMAP_ERROR_RESOLVE          = 17,  // host name resolution failed
    D_IMAP_ERROR_CONNECT          = 18,  // refused or unreachable
    D_IMAP_ERROR_TLS              = 19,  // TLS handshake or verification
    D_IMAP_ERROR_TIMEOUT          = 20,  // an operation timed out
    D_IMAP_ERROR_CLOSED           = 21,  // the connection closed without BYE
    D_IMAP_ERROR_IO               = 22,  // any other transport failure
    // resources and faults
    D_IMAP_ERROR_MEMORY           = 23,  // allocation failed
    D_IMAP_ERROR_INTERNAL         = 24   // a backend fault or unmapped error
};

// 2.1.2
// D_IMAP_ERROR_COUNT
//   constant: the number of enumerators in d_imap_error.
#define D_IMAP_ERROR_COUNT 25

// 2.2    Error queries
//------------------------------------------------------------------------------
// error queries -- none allocate or fail. Names are the enumerator suffix in
// lower case ("invalid_argument"); messages are short sentences for people.
// An unknown value reads "unknown" and belongs to neither class.
const char* d_imap_error_name(enum d_imap_error _error);
const char* d_imap_error_message(enum d_imap_error _error);
bool        d_imap_error_is_server(enum d_imap_error _error);
bool        d_imap_error_is_transport(enum d_imap_error _error);


//==============================================================================
// 3.  PROTOCOL CONSTANTS
//==============================================================================


// 3.1    Ports
//------------------------------------------------------------------------------
// 3.1.1
// D_IMAP_PORT
//   constant: the IMAP port, for a plaintext session or one upgraded with
// STARTTLS.
#define D_IMAP_PORT     143

// 3.1.2
// D_IMAP_PORT_TLS
//   constant: the IMAPS port, for implicit TLS (RFC 8314).
#define D_IMAP_PORT_TLS 993

// 3.2    Limits
//------------------------------------------------------------------------------
// 3.2.1
// D_IMAP_NUMBER_MAX
//   constant: the largest `number` / `nz-number`, which IMAP defines as an
// unsigned 32-bit value: sequence numbers, UIDs, and most counts.
#define D_IMAP_NUMBER_MAX                  UINT32_C(4294967295)

// 3.2.2
// D_IMAP_NUMBER64_MAX
//   constant: the largest `number64` (RFC 9051), a 63-bit value used for
// sizes, literal lengths, and mod-sequences.
#define D_IMAP_NUMBER64_MAX                UINT64_C(9223372036854775807)

// 3.2.3
// D_IMAP_COMMAND_LINE_LIMIT
//   constant: the length a client should keep one command line within,
// quoted strings included and literals not: RFC 7162 section 4 recommends
// about 8192 octets, and asks servers to accept at least that. A command that
// cannot fit should be split.
#define D_IMAP_COMMAND_LINE_LIMIT          UINT32_C(8192)

// 3.2.4
// D_IMAP_RESPONSE_LINE_LIMIT_DEFAULT
//   constant: the default bound on one response line a backend will buffer,
// literals excluded (1 MiB). Server lines have no protocol limit, and a SEARCH
// over a large mailbox can run long, so this is a memory-safety bound;
// d_imap_options.line_limit replaces it per connection.
#define D_IMAP_RESPONSE_LINE_LIMIT_DEFAULT (UINT32_C(1024) * 1024u)

// 3.2.5
// D_IMAP_LITERAL_LIMIT_DEFAULT
//   constant: the default bound on one literal a backend will buffer
// (64 MiB): a memory-safety bound, not a protocol limit, that clears common
// provider message caps. d_imap_options.literal_limit replaces it.
#define D_IMAP_LITERAL_LIMIT_DEFAULT       (UINT64_C(64) * 1024u * 1024u)

// 3.2.6
// D_IMAP_LITERAL_MINUS_MAX
//   constant: the largest literal LITERAL- (RFC 7888) lets a client send
// non-synchronizing.
#define D_IMAP_LITERAL_MINUS_MAX           4096u


//==============================================================================
// 4.  SESSION
//==============================================================================
// The connection state machine of RFC 9051 section 3, the commands and the
// states each is valid in, and the tags a client prefixes to its commands.


// 4.1    Connection states
//------------------------------------------------------------------------------
// 4.1.1
// d_imap_state
//   enum: where a session stands. `disconnected` precedes the greeting and is
// not one of the protocol's four states; `logout` is entered through LOGOUT,
// BYE, or the connection closing.
enum d_imap_state
{
    D_IMAP_STATE_DISCONNECTED      = 0,
    D_IMAP_STATE_NOT_AUTHENTICATED = 1,
    D_IMAP_STATE_AUTHENTICATED     = 2,
    D_IMAP_STATE_SELECTED          = 3,
    D_IMAP_STATE_LOGOUT            = 4
};

// 4.1.2
// D_IMAP_STATE_COUNT
//   constant: the number of enumerators in d_imap_state.
#define D_IMAP_STATE_COUNT 5

// 4.2    Commands
//------------------------------------------------------------------------------
// 4.2.1
// d_imap_command
//   enum: the commands of IMAP4rev1 and IMAP4rev2 and of the extensions most
// servers deploy, grouped by the state each is valid in. A command of the
// authenticated state is valid when selected too, except ENABLE.
enum d_imap_command
{
    D_IMAP_COMMAND_UNKNOWN      = 0,
    // any state: RFC 9051 section 6.1, and ID (RFC 2971)
    D_IMAP_COMMAND_CAPABILITY   = 1,
    D_IMAP_COMMAND_NOOP         = 2,
    D_IMAP_COMMAND_LOGOUT       = 3,
    D_IMAP_COMMAND_ID           = 4,
    // not authenticated: section 6.2
    D_IMAP_COMMAND_STARTTLS     = 5,
    D_IMAP_COMMAND_AUTHENTICATE = 6,
    D_IMAP_COMMAND_LOGIN        = 7,
    // authenticated: section 6.3; LSUB is IMAP4rev1 only (RFC 3501)
    D_IMAP_COMMAND_ENABLE       = 8,
    D_IMAP_COMMAND_SELECT       = 9,
    D_IMAP_COMMAND_EXAMINE      = 10,
    D_IMAP_COMMAND_CREATE       = 11,
    D_IMAP_COMMAND_DELETE       = 12,
    D_IMAP_COMMAND_RENAME       = 13,
    D_IMAP_COMMAND_SUBSCRIBE    = 14,
    D_IMAP_COMMAND_UNSUBSCRIBE  = 15,
    D_IMAP_COMMAND_LIST         = 16,
    D_IMAP_COMMAND_LSUB         = 17,
    D_IMAP_COMMAND_NAMESPACE    = 18,
    D_IMAP_COMMAND_STATUS       = 19,
    D_IMAP_COMMAND_APPEND       = 20,
    D_IMAP_COMMAND_IDLE         = 21,
    // selected: section 6.4; CHECK is IMAP4rev1 only, and SORT and THREAD
    // are RFC 5256
    D_IMAP_COMMAND_CHECK        = 22,
    D_IMAP_COMMAND_CLOSE        = 23,
    D_IMAP_COMMAND_UNSELECT     = 24,
    D_IMAP_COMMAND_EXPUNGE      = 25,
    D_IMAP_COMMAND_SEARCH       = 26,
    D_IMAP_COMMAND_FETCH        = 27,
    D_IMAP_COMMAND_STORE        = 28,
    D_IMAP_COMMAND_COPY         = 29,
    D_IMAP_COMMAND_MOVE         = 30,
    D_IMAP_COMMAND_UID          = 31,
    D_IMAP_COMMAND_SORT         = 32,
    D_IMAP_COMMAND_THREAD       = 33
};

// 4.2.2
// D_IMAP_COMMAND_COUNT
//   constant: the number of enumerators in d_imap_command.
#define D_IMAP_COMMAND_COUNT 34

// 4.3    State and command queries
//------------------------------------------------------------------------------
// state and command queries -- none allocate or fail. A state name is a
// lower-case description; a command name is its upper-case keyword, and the
// empty string for D_IMAP_COMMAND_UNKNOWN or any other value without one.
// _from_name matches case-insensitively and yields D_IMAP_COMMAND_UNKNOWN for
// anything else. _is_valid_in applies RFC 9051 section 3, with ENABLE
// confined to the authenticated state.
const char*         d_imap_state_name(enum d_imap_state _state);
const char*         d_imap_command_name(enum d_imap_command _command);
enum d_imap_command d_imap_command_from_name(struct d_pack_text _name);
bool                d_imap_command_is_valid_in(enum d_imap_command _command,
                                               enum d_imap_state   _state);

// 4.4    Tags
//------------------------------------------------------------------------------
// 4.4.1
// D_IMAP_TAG_PREFIX_MAX
//   constant: the longest prefix a tagger accepts, in octets.
#define D_IMAP_TAG_PREFIX_MAX 8

// 4.4.2
// D_IMAP_TAG_CAPACITY
//   constant: the storage a d_imap_tag reserves: the longest prefix, the ten
// digits of the largest counter, and a terminating NUL, rounded up.
#define D_IMAP_TAG_CAPACITY   20

// 4.4.3
// d_imap_tag
//   struct: one issued tag, NUL-terminated so that it can also be printed.
struct d_imap_tag
{
    uint32_t length;                     // octets in text, without the NUL
    char     text[D_IMAP_TAG_CAPACITY];  // the tag
};

// 4.4.4
// d_imap_tagger
//   struct: a tag generator: a fixed prefix and a counter, issuing A0001,
// A0002, and so on. A tag must be unique among a session's outstanding
// commands, which a counter guarantees until it wraps after 4294967295.
struct d_imap_tagger
{
    uint32_t next;                          // counter for the next tag
    uint32_t prefix_length;                 // octets in prefix
    char     prefix[D_IMAP_TAG_PREFIX_MAX]; // not NUL-terminated
};

// 4.5    Tag operations
//------------------------------------------------------------------------------
/**
 * @brief Prepares a tagger to issue tags with the given prefix.
 *
 * @param[out] _tagger  the tagger to initialize.
 * @param[in]  _prefix  up to D_IMAP_TAG_PREFIX_MAX tag characters (any
 *                      ASTRING-CHAR except "+"); empty selects "A".
 * @post   on success the first tag issued is the prefix followed by "0001".
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_tagger` is
 *         `NULL`, or `_prefix` is too long or holds a character no tag may.
 */
enum d_imap_error  d_imap_tagger_init(struct d_imap_tagger* _tagger,
                                      struct d_pack_text    _prefix);
/**
 * @brief Issues the next tag and advances the counter.
 *
 * @note the counter is zero-padded to four digits and widens as needed;
 *       after 4294967295 it wraps to 1.
 *
 * @param[in,out] _tagger  an initialized tagger.
 * @param[out]    _tag     receives the tag.
 * @return D_IMAP_ERROR_NONE, or D_IMAP_ERROR_INVALID_ARGUMENT if either
 *         pointer is `NULL` or `_tagger` is corrupt.
 */
enum d_imap_error  d_imap_tagger_next(struct d_imap_tagger* _tagger,
                                      struct d_imap_tag*    _tag);
// tag views -- none allocate or fail. A tag compares case-sensitively, since
// the server echoes it octet for octet; a NULL tag matches nothing.
struct d_pack_text d_imap_tag_text(const struct d_imap_tag* _tag);
bool               d_imap_tag_matches(const struct d_imap_tag* _tag,
                                      struct d_pack_text       _candidate);


//==============================================================================
// 5.  SECURITY AND AUTHENTICATION
//==============================================================================
// How a connection is protected, and the SASL mechanisms (RFC 4422) that
// AUTHENTICATE names, together with the LOGIN command, which is not SASL but
// is chosen among them.


// 5.1    Transport security
//------------------------------------------------------------------------------
// 5.1.1
// d_imap_security
//   enum: how a session protects itself. Zero is implicit TLS, the choice
// RFC 8314 recommends, so a zero-initialized d_imap_options is secure.
// `opportunistic` upgrades only when the server offers STARTTLS, which an
// active attacker can suppress; `none` is for loopback and test servers.
enum d_imap_security
{
    D_IMAP_SECURITY_TLS           = 0,  // TLS from the first octet; port 993
    D_IMAP_SECURITY_STARTTLS      = 1,  // STARTTLS required; fail without it
    D_IMAP_SECURITY_OPPORTUNISTIC = 2,  // STARTTLS if offered, else plaintext
    D_IMAP_SECURITY_NONE          = 3   // plaintext throughout
};

// 5.1.2
// D_IMAP_SECURITY_COUNT
//   constant: the number of enumerators in d_imap_security.
#define D_IMAP_SECURITY_COUNT 4

// 5.2    Authentication mechanisms
//------------------------------------------------------------------------------
// 5.2.1
// d_imap_auth_mechanism
//   enum: a way to authenticate. Every value but the first is a SASL
// mechanism, named as it appears after "AUTH=" in a capability list; the
// first is the LOGIN command. A set of them is a uint32_t of D_IMAP_AUTH_BIT
// values.
enum d_imap_auth_mechanism
{
    D_IMAP_AUTH_LOGIN_COMMAND      = 0,   // the LOGIN command, not SASL
    D_IMAP_AUTH_PLAIN              = 1,   // RFC 4616
    D_IMAP_AUTH_LOGIN              = 2,   // an expired draft, still common
    D_IMAP_AUTH_CRAM_MD5           = 3,   // RFC 2195; weak
    D_IMAP_AUTH_DIGEST_MD5         = 4,   // RFC 2831; historic (RFC 6331)
    D_IMAP_AUTH_SCRAM_SHA_1        = 5,   // RFC 5802
    D_IMAP_AUTH_SCRAM_SHA_1_PLUS   = 6,   // RFC 5802, with channel binding
    D_IMAP_AUTH_SCRAM_SHA_256      = 7,   // RFC 7677
    D_IMAP_AUTH_SCRAM_SHA_256_PLUS = 8,   // RFC 7677, with channel binding
    D_IMAP_AUTH_XOAUTH2            = 9,   // Google and Microsoft OAuth 2.0
    D_IMAP_AUTH_OAUTHBEARER        = 10,  // RFC 7628
    D_IMAP_AUTH_GSSAPI             = 11,  // RFC 4752 (Kerberos)
    D_IMAP_AUTH_EXTERNAL           = 12,  // RFC 4422 appendix A
    D_IMAP_AUTH_NTLM               = 13,  // Microsoft; weak
    D_IMAP_AUTH_ANONYMOUS          = 14   // RFC 4505
};

// 5.2.2
// D_IMAP_AUTH_COUNT
//   constant: the number of enumerators in d_imap_auth_mechanism.
#define D_IMAP_AUTH_COUNT 15

// 5.2.3
// D_IMAP_AUTH_BIT
//   macro: the set bit for mechanism `_mechanism`.
#define D_IMAP_AUTH_BIT(_mechanism) ((uint32_t)1 << (_mechanism))

// 5.2.4
// D_IMAP_AUTH_DEFAULT_SET
//   constant: the mechanisms permitted when the options name none: the SCRAM
// family, both OAuth mechanisms, PLAIN, the LOGIN command, and SASL LOGIN.
// The weak mechanisms, and those needing context an option block cannot hold
// (GSSAPI, EXTERNAL, ANONYMOUS), must be permitted by name.
#define D_IMAP_AUTH_DEFAULT_SET                                                \
    ( D_IMAP_AUTH_BIT(D_IMAP_AUTH_LOGIN_COMMAND)      |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_PLAIN)              |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_LOGIN)              |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_1)        |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_1_PLUS)   |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_256)      |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_256_PLUS) |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_XOAUTH2)            |                        \
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_OAUTHBEARER) )

// 5.3    Security and mechanism queries
//------------------------------------------------------------------------------
// security and mechanism queries -- none allocate or fail. A security name is
// a lower-case description, "unknown" for an unknown value; its default port
// is 993 for implicit TLS, 143 otherwise, and 0 for an unknown value. A
// mechanism name is its SASL name ("SCRAM-SHA-256"), empty for an unknown
// value; the LOGIN command's is "login-command", a description rather than a
// wire name.
const char* d_imap_security_name(enum d_imap_security _security);
uint16_t    d_imap_security_default_port(enum d_imap_security _security);
const char* d_imap_auth_name(enum d_imap_auth_mechanism _mechanism);
/**
 * @brief Looks up a SASL mechanism by name, case-insensitively.
 *
 * @param[in]  _name       the mechanism name, without "AUTH=".
 * @param[out] _mechanism  receives the mechanism when found.
 * @return `true` if `_name` is a mechanism this vocabulary names; `false`
 *         otherwise, or if `_mechanism` is `NULL`. The LOGIN command is not
 *         a SASL mechanism and is never found.
 */
bool        d_imap_auth_from_name(struct d_pack_text          _name,
                                  enum d_imap_auth_mechanism* _mechanism);


//==============================================================================
// 6.  CAPABILITIES
//==============================================================================
// What a server advertises: in its greeting, in a CAPABILITY response, or in a
// [CAPABILITY ...] response code. A parse replaces a set rather than merging
// into it, since each advertisement is complete, and the one after STARTTLS
// or authentication supersedes the one before.


// 6.1    Capability vocabulary
//------------------------------------------------------------------------------
// 6.1.1
// d_imap_capability
//   enum: the capabilities this vocabulary names, each commented with its
// defining RFC; d_imap_capability_name() gives the wire spelling. A set of
// them is a uint64_t of D_IMAP_CAPABILITY_BIT values.
enum d_imap_capability
{
    D_IMAP_CAPABILITY_IMAP4REV1             = 0,   // RFC 3501
    D_IMAP_CAPABILITY_IMAP4REV2             = 1,   // RFC 9051
    D_IMAP_CAPABILITY_STARTTLS              = 2,   // RFC 3501
    D_IMAP_CAPABILITY_LOGINDISABLED         = 3,   // RFC 3501
    D_IMAP_CAPABILITY_SASL_IR               = 4,   // RFC 4959
    D_IMAP_CAPABILITY_IDLE                  = 5,   // RFC 2177
    D_IMAP_CAPABILITY_NAMESPACE             = 6,   // RFC 2342
    D_IMAP_CAPABILITY_UIDPLUS               = 7,   // RFC 4315
    D_IMAP_CAPABILITY_MOVE                  = 8,   // RFC 6851
    D_IMAP_CAPABILITY_UNSELECT              = 9,   // RFC 3691
    D_IMAP_CAPABILITY_ENABLE                = 10,  // RFC 5161
    D_IMAP_CAPABILITY_CONDSTORE             = 11,  // RFC 7162
    D_IMAP_CAPABILITY_QRESYNC               = 12,  // RFC 7162
    D_IMAP_CAPABILITY_LITERAL_PLUS          = 13,  // RFC 7888
    D_IMAP_CAPABILITY_LITERAL_MINUS         = 14,  // RFC 7888
    D_IMAP_CAPABILITY_ID                    = 15,  // RFC 2971
    D_IMAP_CAPABILITY_CHILDREN              = 16,  // RFC 3348
    D_IMAP_CAPABILITY_SPECIAL_USE           = 17,  // RFC 6154
    D_IMAP_CAPABILITY_CREATE_SPECIAL_USE    = 18,  // RFC 6154
    D_IMAP_CAPABILITY_LIST_EXTENDED         = 19,  // RFC 5258
    D_IMAP_CAPABILITY_LIST_STATUS           = 20,  // RFC 5819
    D_IMAP_CAPABILITY_ESEARCH               = 21,  // RFC 4731
    D_IMAP_CAPABILITY_SEARCHRES             = 22,  // RFC 5182
    D_IMAP_CAPABILITY_SORT                  = 23,  // RFC 5256
    D_IMAP_CAPABILITY_THREAD_ORDEREDSUBJECT = 24,  // RFC 5256
    D_IMAP_CAPABILITY_THREAD_REFERENCES     = 25,  // RFC 5256
    D_IMAP_CAPABILITY_COMPRESS_DEFLATE      = 26,  // RFC 4978
    D_IMAP_CAPABILITY_UTF8_ACCEPT           = 27,  // RFC 6855
    D_IMAP_CAPABILITY_UTF8_ONLY             = 28,  // RFC 6855
    D_IMAP_CAPABILITY_QUOTA                 = 29,  // RFC 9208
    D_IMAP_CAPABILITY_ACL                   = 30,  // RFC 4314
    D_IMAP_CAPABILITY_BINARY                = 31,  // RFC 3516
    D_IMAP_CAPABILITY_CATENATE              = 32,  // RFC 4469
    D_IMAP_CAPABILITY_MULTIAPPEND           = 33,  // RFC 3502
    D_IMAP_CAPABILITY_WITHIN                = 34,  // RFC 5032
    D_IMAP_CAPABILITY_METADATA              = 35,  // RFC 5464
    D_IMAP_CAPABILITY_METADATA_SERVER       = 36,  // RFC 5464
    D_IMAP_CAPABILITY_NOTIFY                = 37,  // RFC 5465
    D_IMAP_CAPABILITY_OBJECTID              = 38,  // RFC 8474
    D_IMAP_CAPABILITY_SAVEDATE              = 39,  // RFC 8514
    D_IMAP_CAPABILITY_STATUS_SIZE           = 40,  // RFC 8438
    D_IMAP_CAPABILITY_APPENDLIMIT           = 41,  // RFC 7889
    D_IMAP_CAPABILITY_PREVIEW               = 42,  // RFC 8970
    D_IMAP_CAPABILITY_UNAUTHENTICATE        = 43,  // RFC 8437
    D_IMAP_CAPABILITY_LIST_MYRIGHTS         = 44,  // RFC 8440
    D_IMAP_CAPABILITY_REPLACE               = 45   // RFC 8508
};

// 6.1.2
// D_IMAP_CAPABILITY_COUNT
//   constant: the number of enumerators in d_imap_capability.
#define D_IMAP_CAPABILITY_COUNT 46

// 6.1.3
// D_IMAP_CAPABILITY_BIT
//   macro: the set bit for capability `_capability`.
#define D_IMAP_CAPABILITY_BIT(_capability) ((uint64_t)1 << (_capability))

// 6.1.4
// d_imap_capabilities
//   struct: one capability advertisement. A small value, passed by value to
// queries.
struct d_imap_capabilities
{
    uint64_t set;           // D_IMAP_CAPABILITY_BIT of each one advertised
    uint64_t append_limit;  // the n of APPENDLIMIT=n; 0 when absent or bare
    uint32_t auth;          // D_IMAP_AUTH_BIT of each AUTH= mechanism
    uint32_t unknown;       // advertisements outside this vocabulary
};

// 6.2    Capability operations
//------------------------------------------------------------------------------
// capability queries -- none allocate or fail. A name is the wire spelling
// ("LITERAL+", "THREAD=REFERENCES"), and empty for an unknown value.
const char*       d_imap_capability_name(enum d_imap_capability _capability);
bool              d_imap_capabilities_has(struct d_imap_capabilities _caps,
                                          enum d_imap_capability     _cap);
bool              d_imap_capabilities_has_auth(
                      struct d_imap_capabilities _caps,
                      enum d_imap_auth_mechanism _mechanism);
/**
 * @brief Looks up a capability by its wire name, case-insensitively.
 *
 * @note "APPENDLIMIT=n" is not a name; the parser splits off its value.
 *
 * @param[in]  _name        the capability name.
 * @param[out] _capability  receives the capability when found.
 * @return `true` if `_name` is in the vocabulary; `false` otherwise, or if
 *         `_capability` is `NULL`.
 */
bool              d_imap_capability_from_name(
                      struct d_pack_text      _name,
                      enum d_imap_capability* _capability);
/**
 * @brief Parses a space-separated capability list into a set.
 *
 * @note the list is what follows "CAPABILITY" in an untagged response, or the
 *       argument of a [CAPABILITY ...] response code; a trailing CRLF is
 *       accepted. AUTH= entries fill `auth`, APPENDLIMIT=n fills
 *       `append_limit`, and names outside the vocabulary count in `unknown`.
 *
 * @param[in]  _list  the capability list.
 * @param[out] _caps  receives the set; untouched on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_caps` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX if an entry is not an atom or an
 *         APPENDLIMIT value is not a number; D_IMAP_ERROR_RANGE if that
 *         number exceeds D_IMAP_NUMBER64_MAX.
 */
enum d_imap_error d_imap_capabilities_parse(struct d_pack_text          _list,
                                            struct d_imap_capabilities* _caps);


//==============================================================================
// 7.  OPTIONS
//==============================================================================
// The connection parameters every backend accepts, so that one option block
// configures any of them identically. Zero is the default for every field,
// and the defaults are secure: implicit TLS, full certificate verification,
// and no credential sent in the clear.


// 7.1    Option flags
//------------------------------------------------------------------------------
// 7.1.1
// D_IMAP_OPTION_NO_VERIFY_PEER
//   constant: skip certificate-chain verification; for test servers with
// self-signed certificates only.
#define D_IMAP_OPTION_NO_VERIFY_PEER       ((uint32_t)1 << 0)

// 7.1.2
// D_IMAP_OPTION_NO_VERIFY_HOST
//   constant: skip matching the certificate against the host name.
#define D_IMAP_OPTION_NO_VERIFY_HOST       ((uint32_t)1 << 1)

// 7.1.3
// D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH
//   constant: permit the mechanisms that expose the password or token (the
// LOGIN command, PLAIN, LOGIN, XOAUTH2, OAUTHBEARER) on a connection without
// TLS.
#define D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH ((uint32_t)1 << 2)

// 7.2    Option block
//------------------------------------------------------------------------------
// 7.2.1
// d_imap_options
//   struct: one connection's parameters. The option block borrows its text
// and owns nothing, so it must not outlive the strings it points at. The
// password serves the password mechanisms and the token the OAuth ones; the
// authzid, usually empty, asks to act as another identity.
struct d_imap_options
{
    uint64_t           literal_limit;       // 0: D_IMAP_LITERAL_LIMIT_DEFAULT
    uint32_t           port;                // 0: the security mode's default
    int32_t            security;            // enum d_imap_security
    uint32_t           auth_mechanisms;     // 0: D_IMAP_AUTH_DEFAULT_SET
    uint32_t           flags;               // D_IMAP_OPTION_* bits
    uint32_t           connect_timeout_ms;  // 0: the backend's default
    uint32_t           io_timeout_ms;       // 0: the backend's default
    uint32_t           line_limit;          // 0: the default line bound
    struct d_pack_text host;                // server name or address
    struct d_pack_text username;            // authentication identity
    struct d_pack_text password;            // password mechanisms' secret
    struct d_pack_text token;               // OAuth 2.0 bearer token
    struct d_pack_text authzid;             // authorization identity
};

// 7.3    Option operations
//------------------------------------------------------------------------------
/**
 * @brief Resets every field to its default, which is zero.
 *
 * @param[out] _options  the block to reset; `NULL` does nothing.
 */
void              d_imap_options_init(struct d_imap_options* _options);
/**
 * @brief Checks an option block for values no backend could honor.
 *
 * @note the checks: a known security mode; a port of at most 65535; a
 *       non-empty host without spaces or control characters; no unknown
 *       mechanism or option bit; no NUL in a credential; a username whenever
 *       a password is given; and, under D_IMAP_SECURITY_NONE, no password or
 *       token unless D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH is set.
 *
 * @param[in] _options  the block to check.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a malformed
 *         value; D_IMAP_ERROR_RANGE for a port above 65535;
 *         D_IMAP_ERROR_SECURITY for a credential that could only travel in
 *         the clear.
 */
enum d_imap_error d_imap_options_validate(
                      const struct d_imap_options* _options);
/**
 * @brief Resolves the port to connect to.
 *
 * @param[in] _options  the connection options.
 * @return the explicit port if one is set, otherwise the security mode's
 *         default; `0` if `_options` is `NULL` or its port or security mode is
 *         out of range.
 */
uint16_t          d_imap_options_port(const struct d_imap_options* _options);
/**
 * @brief Chooses how to authenticate, identically for every backend.
 *
 * @note the candidates are the permitted mechanisms (the default set when the
 *       options name none) that the backend supports, that the server
 *       advertises (for the LOGIN command: that LOGINDISABLED is absent), and
 *       whose credential the options hold. Channel binding needs `_secure`;
 *       so do the mechanisms that expose the secret, unless
 *       D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH is set. The strongest candidate
 *       wins, in this order: SCRAM-SHA-256-PLUS, SCRAM-SHA-256,
 *       SCRAM-SHA-1-PLUS, SCRAM-SHA-1, EXTERNAL, GSSAPI, OAUTHBEARER,
 *       XOAUTH2, PLAIN, the LOGIN command, LOGIN, CRAM-MD5, DIGEST-MD5,
 *       NTLM, ANONYMOUS.
 *
 * @param[in]  _options    the connection options.
 * @param[in]  _caps       the server's latest capability advertisement.
 * @param[in]  _supported  the D_IMAP_AUTH_BIT set the backend implements.
 * @param[in]  _secure     whether TLS protects the connection now.
 * @param[out] _mechanism  receives the choice on success.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if a pointer is
 *         `NULL`; D_IMAP_ERROR_SECURITY if every candidate needs TLS the
 *         connection lacks; D_IMAP_ERROR_UNSUPPORTED if there is none.
 */
enum d_imap_error d_imap_auth_select(const struct d_imap_options* _options,
                                     struct d_imap_capabilities   _caps,
                                     uint32_t                     _supported,
                                     bool                         _secure,
                                     enum d_imap_auth_mechanism*  _mechanism);


//==============================================================================
// 8.  MESSAGE AND MAILBOX ATTRIBUTES
//==============================================================================
// The flags on a message, the attributes on a mailbox name, and the items a
// STATUS command asks for. Names compare case-insensitively; anything outside
// a vocabulary is counted, not rejected, since servers extend all three.


// 8.1    Message flags
//------------------------------------------------------------------------------
// 8.1.1
// d_imap_flag
//   enum: the system flags, the "\*" of PERMANENTFLAGS (new keywords may be
// created), and the keywords RFC 9051 section 2.3.2 lists. \Recent and \*
// are set by servers only, and \Recent exists in IMAP4rev1 only. A set of
// them is a uint32_t of D_IMAP_FLAG_BIT values.
enum d_imap_flag
{
    D_IMAP_FLAG_SEEN      = 0,   // \Seen
    D_IMAP_FLAG_ANSWERED  = 1,   // \Answered
    D_IMAP_FLAG_FLAGGED   = 2,   // \Flagged
    D_IMAP_FLAG_DELETED   = 3,   // \Deleted
    D_IMAP_FLAG_DRAFT     = 4,   // \Draft
    D_IMAP_FLAG_RECENT    = 5,   // \Recent
    D_IMAP_FLAG_WILDCARD  = 6,   // \*
    D_IMAP_FLAG_FORWARDED = 7,   // $Forwarded
    D_IMAP_FLAG_MDNSENT   = 8,   // $MDNSent
    D_IMAP_FLAG_JUNK      = 9,   // $Junk
    D_IMAP_FLAG_NOTJUNK   = 10,  // $NotJunk
    D_IMAP_FLAG_PHISHING  = 11   // $Phishing
};

// 8.1.2
// D_IMAP_FLAG_COUNT
//   constant: the number of enumerators in d_imap_flag.
#define D_IMAP_FLAG_COUNT 12

// 8.1.3
// D_IMAP_FLAG_BIT
//   macro: the set bit for flag `_flag`.
#define D_IMAP_FLAG_BIT(_flag) ((uint32_t)1 << (_flag))

// 8.2    Mailbox attributes
//------------------------------------------------------------------------------
// 8.2.1
// d_imap_mailbox_attribute
//   enum: the attributes a LIST response puts on a mailbox name: RFC 3501's
// four, CHILDREN (RFC 3348), LIST-EXTENDED (RFC 5258), and the special uses
// of RFC 6154 and RFC 8457. A set of them is a uint32_t of D_IMAP_MAILBOX_BIT
// values.
enum d_imap_mailbox_attribute
{
    D_IMAP_MAILBOX_NOINFERIORS   = 0,   // \Noinferiors
    D_IMAP_MAILBOX_NOSELECT      = 1,   // \Noselect
    D_IMAP_MAILBOX_MARKED        = 2,   // \Marked
    D_IMAP_MAILBOX_UNMARKED      = 3,   // \Unmarked
    D_IMAP_MAILBOX_HASCHILDREN   = 4,   // \HasChildren
    D_IMAP_MAILBOX_HASNOCHILDREN = 5,   // \HasNoChildren
    D_IMAP_MAILBOX_NONEXISTENT   = 6,   // \NonExistent
    D_IMAP_MAILBOX_SUBSCRIBED    = 7,   // \Subscribed
    D_IMAP_MAILBOX_REMOTE        = 8,   // \Remote
    D_IMAP_MAILBOX_ALL           = 9,   // \All
    D_IMAP_MAILBOX_ARCHIVE       = 10,  // \Archive
    D_IMAP_MAILBOX_DRAFTS        = 11,  // \Drafts
    D_IMAP_MAILBOX_FLAGGED       = 12,  // \Flagged
    D_IMAP_MAILBOX_JUNK          = 13,  // \Junk
    D_IMAP_MAILBOX_SENT          = 14,  // \Sent
    D_IMAP_MAILBOX_TRASH         = 15,  // \Trash
    D_IMAP_MAILBOX_IMPORTANT     = 16   // \Important
};

// 8.2.2
// D_IMAP_MAILBOX_ATTRIBUTE_COUNT
//   constant: the number of enumerators in d_imap_mailbox_attribute.
#define D_IMAP_MAILBOX_ATTRIBUTE_COUNT 17

// 8.2.3
// D_IMAP_MAILBOX_BIT
//   macro: the set bit for mailbox attribute `_attribute`.
#define D_IMAP_MAILBOX_BIT(_attribute) ((uint32_t)1 << (_attribute))

// 8.2.4
// D_IMAP_MAILBOX_SPECIAL_USE
//   constant: the set of special-use attributes, \All through \Important;
// AND a mailbox's attributes with it to learn whether it has a special use.
#define D_IMAP_MAILBOX_SPECIAL_USE                                             \
    ( D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_ALL)     |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_ARCHIVE) |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_DRAFTS)  |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_FLAGGED) |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_JUNK)    |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_SENT)    |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_TRASH)   |                             \
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_IMPORTANT) )

// 8.3    STATUS items
//------------------------------------------------------------------------------
// 8.3.1
// d_imap_status_item
//   enum: the numeric items STATUS can report. RECENT exists in IMAP4rev1
// only; DELETED and SIZE are IMAP4rev2 (SIZE also RFC 8438); HIGHESTMODSEQ is
// RFC 7162. A set of them is a uint32_t of D_IMAP_STATUS_BIT values.
enum d_imap_status_item
{
    D_IMAP_STATUS_MESSAGES      = 0,
    D_IMAP_STATUS_RECENT        = 1,
    D_IMAP_STATUS_UIDNEXT       = 2,
    D_IMAP_STATUS_UIDVALIDITY   = 3,
    D_IMAP_STATUS_UNSEEN        = 4,
    D_IMAP_STATUS_DELETED       = 5,
    D_IMAP_STATUS_SIZE          = 6,
    D_IMAP_STATUS_HIGHESTMODSEQ = 7
};

// 8.3.2
// D_IMAP_STATUS_ITEM_COUNT
//   constant: the number of enumerators in d_imap_status_item.
#define D_IMAP_STATUS_ITEM_COUNT 8

// 8.3.3
// D_IMAP_STATUS_BIT
//   macro: the set bit for STATUS item `_item`.
#define D_IMAP_STATUS_BIT(_item) ((uint32_t)1 << (_item))

// 8.3.4
// d_imap_status_data
//   struct: the items one STATUS response carried. A field is meaningful only
// when its bit is in `present`.
struct d_imap_status_data
{
    uint64_t size;           // SIZE: total octets in the mailbox
    uint64_t highestmodseq;  // HIGHESTMODSEQ
    uint32_t present;        // D_IMAP_STATUS_BIT of each item present
    uint32_t messages;       // MESSAGES
    uint32_t recent;         // RECENT
    uint32_t uidnext;        // UIDNEXT
    uint32_t uidvalidity;    // UIDVALIDITY
    uint32_t unseen;         // UNSEEN
    uint32_t deleted;        // DELETED
};

// 8.4    Attribute operations
//------------------------------------------------------------------------------
// attribute names -- none allocate or fail. A flag or mailbox attribute name
// carries its backslash or dollar ("\Seen", "$Junk"); a STATUS item name is
// its keyword. An unknown value's name is empty.
const char*       d_imap_flag_name(enum d_imap_flag _flag);
const char*       d_imap_mailbox_attribute_name(
                      enum d_imap_mailbox_attribute _attribute);
const char*       d_imap_status_item_name(enum d_imap_status_item _item);
// attribute lookups -- case-insensitive, and none allocate. Each returns
// `false` for a name outside its vocabulary or a NULL out-pointer.
bool              d_imap_flag_from_name(struct d_pack_text _name,
                                        enum d_imap_flag*  _flag);
bool              d_imap_mailbox_attribute_from_name(
                      struct d_pack_text             _name,
                      enum d_imap_mailbox_attribute* _attribute);
bool              d_imap_status_item_from_name(
                      struct d_pack_text       _name,
                      enum d_imap_status_item* _item);
/**
 * @brief Parses a parenthesized flag list, as in FLAGS, PERMANENTFLAGS, or a
 *        FETCH response's FLAGS item.
 *
 * @param[in]  _list     the list, "(" through ")", optionally followed by
 *                       CRLF.
 * @param[out] _set      receives the D_IMAP_FLAG_BIT set of known flags.
 * @param[out] _unknown  receives the count of other flags and keywords; may
 *                       be `NULL`.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_set` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX if `_list` is not a flag list.
 */
enum d_imap_error d_imap_flags_parse(struct d_pack_text _list,
                                     uint32_t*          _set,
                                     uint32_t*          _unknown);
/**
 * @brief Writes a flag set as a parenthesized list, in enumeration order.
 *
 * @note an empty set writes "()". \Recent and \* are written if present,
 *       although only a server may send them.
 *
 * @param[in] _sink  the destination.
 * @param[in] _set   a D_IMAP_FLAG_BIT set.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if the sink has
 *         no write function or `_set` holds an unknown bit;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_flags_write(struct d_pack_sink _sink,
                                     uint32_t           _set);
/**
 * @brief Parses the parenthesized attribute list of a LIST or LSUB response.
 *
 * @param[in]  _list     the list, "(" through ")", optionally followed by
 *                       CRLF.
 * @param[out] _set      receives the D_IMAP_MAILBOX_BIT set of known
 *                       attributes.
 * @param[out] _unknown  receives the count of other attributes; may be
 *                       `NULL`.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_set` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX if `_list` is not an attribute list.
 */
enum d_imap_error d_imap_mailbox_attributes_parse(struct d_pack_text _list,
                                                  uint32_t*          _set,
                                                  uint32_t*          _unknown);
/**
 * @brief Writes the parenthesized item list of a STATUS command.
 *
 * @param[in] _sink   the destination.
 * @param[in] _items  a non-empty D_IMAP_STATUS_BIT set, written in
 *                    enumeration order.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if the sink has
 *         no write function, or `_items` is empty or holds an unknown bit;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_status_items_write(struct d_pack_sink _sink,
                                            uint32_t           _items);
/**
 * @brief Parses the parenthesized item list of a STATUS response.
 *
 * @note items outside the vocabulary are skipped whatever their value's shape
 *       (number, NIL, string, or list), so an extension cannot break the
 *       parse.
 *
 * @param[in]  _list  the list, "(" through ")", optionally followed by CRLF:
 *                    what follows the mailbox name in "* STATUS".
 * @param[out] _data  receives the items; untouched on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_data` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX if `_list` is malformed or a known item
 *         lacks a number; D_IMAP_ERROR_RANGE if a value exceeds its item's
 *         width; D_IMAP_ERROR_INCOMPLETE if `_list` ends early.
 */
enum d_imap_error d_imap_status_data_parse(struct d_pack_text         _list,
                                           struct d_imap_status_data* _data);


//==============================================================================
// 9.  STRINGS AND MAILBOX NAMES
//==============================================================================
// An IMAP string travels as an atom when every character allows it, as a
// quoted string when no character forbids it, and otherwise as a literal: a
// "{n}" prefix, a line break, and n raw octets. The writers choose the most
// compact form that is valid, and report it, because a synchronizing literal
// obliges the caller to wait for the server's "+" before sending the octets.
// Mailbox names additionally travel in modified UTF-7 (RFC 3501 section
// 5.1.3) unless the session has enabled UTF8=ACCEPT (RFC 6855).
//   Arguments are checked before anything is written, but a sink can refuse
// midway, and a decoder can meet bad input midway; either way the sink may
// hold a prefix of the output. A caller needing all-or-nothing writes into a
// buffer sink first.


// 9.1    String forms
//------------------------------------------------------------------------------
// 9.1.1
// d_imap_string_form
//   enum: how a string was, or would be, written. For the two literal forms
// only the prefix is written: after LITERAL the caller flushes the line,
// waits for a continuation response, and then sends the octets; after
// LITERAL_NONSYNC it sends them at once.
enum d_imap_string_form
{
    D_IMAP_STRING_ATOM            = 0,  // written whole
    D_IMAP_STRING_QUOTED          = 1,  // written whole
    D_IMAP_STRING_LITERAL         = 2,  // "{n}" CRLF written; await "+"
    D_IMAP_STRING_LITERAL_NONSYNC = 3   // "{n+}" CRLF written; send now
};

// 9.1.2
// D_IMAP_STRING_UTF8
//   constant: writer flag: the session enabled UTF8=ACCEPT, so valid UTF-8
// may travel in a quoted string, and mailbox names travel as UTF-8 rather
// than modified UTF-7.
#define D_IMAP_STRING_UTF8           ((uint32_t)1 << 0)

// 9.1.3
// D_IMAP_STRING_LITERAL_PLUS
//   constant: writer flag: the server offers LITERAL+, so every literal may
// be non-synchronizing.
#define D_IMAP_STRING_LITERAL_PLUS   ((uint32_t)1 << 1)

// 9.1.4
// D_IMAP_STRING_LITERAL_MINUS
//   constant: writer flag: the server offers LITERAL-, so literals of at most
// D_IMAP_LITERAL_MINUS_MAX octets may be non-synchronizing.
#define D_IMAP_STRING_LITERAL_MINUS  ((uint32_t)1 << 2)

// 9.1.5
// D_IMAP_STRING_LIST_WILDCARDS
//   constant: writer flag: the string is a LIST pattern, so "%" and "*" are
// wildcards that stay unquoted rather than characters to protect.
#define D_IMAP_STRING_LIST_WILDCARDS ((uint32_t)1 << 3)

// 9.2    String writers
//------------------------------------------------------------------------------
// A string holding NUL cannot be represented at all (a literal excludes it
// without BINARY). The writers are stricter than the grammar in one respect:
// it admits control characters other than CR and LF in quoted strings, which
// some servers reject, so these send such strings as literals instead.
/**
 * @brief Determines the form d_imap_astring_write() would use.
 *
 * @note an atom needs one or more ASTRING-CHARs and is never "NIL", which is
 *       quoted to stay unambiguous; the empty string is quoted.
 *
 * @param[in]  _value  the string.
 * @param[in]  _flags  D_IMAP_STRING_* flags.
 * @param[out] _form   receives the form.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_form` is
 *         `NULL` or `_value` has a length but no data; D_IMAP_ERROR_ENCODING
 *         if `_value` holds a NUL.
 */
enum d_imap_error d_imap_astring_form(struct d_pack_text       _value,
                                      uint32_t                 _flags,
                                      enum d_imap_string_form* _form);
/**
 * @brief Writes a string in its most compact valid form: an atom, a quoted
 *        string, or a literal's prefix.
 *
 * @param[in]  _sink   the destination.
 * @param[in]  _value  the string.
 * @param[in]  _flags  D_IMAP_STRING_* flags.
 * @param[out] _form   receives the form used; may be `NULL`, in which case a
 *                     string needing a literal is refused, not written.
 * @post   for either literal form, the caller still owes the octets of
 *         `_value`; see d_imap_string_form.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function, data-less text, or a literal with no `_form`;
 *         D_IMAP_ERROR_ENCODING if `_value` holds a NUL; D_IMAP_ERROR_SINK if
 *         the sink refuses.
 */
enum d_imap_error d_imap_astring_write(struct d_pack_sink       _sink,
                                       struct d_pack_text       _value,
                                       uint32_t                 _flags,
                                       enum d_imap_string_form* _form);
/**
 * @brief Writes a string as a quoted string, escaping '"' and '\'.
 *
 * @param[in] _sink   the destination.
 * @param[in] _value  the string.
 * @param[in] _flags  D_IMAP_STRING_UTF8 admits valid UTF-8; others ignored.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function or data-less text; D_IMAP_ERROR_ENCODING if
 *         `_value` cannot be quoted; D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_quoted_write(struct d_pack_sink _sink,
                                      struct d_pack_text _value,
                                      uint32_t           _flags);
/**
 * @brief Writes a literal's prefix: "{n}" or "{n+}", then CRLF.
 *
 * @param[in]  _sink   the destination.
 * @param[in]  _size   the octet count that will follow.
 * @param[in]  _flags  D_IMAP_STRING_LITERAL_PLUS or _MINUS permit the
 *                     non-synchronizing form.
 * @param[out] _form   receives which literal form was written; may be
 *                     `NULL`.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function; D_IMAP_ERROR_RANGE if `_size` exceeds
 *         D_IMAP_NUMBER64_MAX; D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_literal_prefix_write(
                      struct d_pack_sink       _sink,
                      uint64_t                 _size,
                      uint32_t                 _flags,
                      enum d_imap_string_form* _form);
/**
 * @brief Writes the value of a quoted string's content, removing escapes.
 *
 * @param[in] _sink     the destination.
 * @param[in] _content  the text between the quotes, still escaped.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function or data-less text; D_IMAP_ERROR_SYNTAX for a
 *         backslash not followed by '"' or '\'; D_IMAP_ERROR_SINK if the
 *         sink refuses.
 */
enum d_imap_error d_imap_quoted_unescape(struct d_pack_sink _sink,
                                         struct d_pack_text _content);

// 9.3    Mailbox names
//------------------------------------------------------------------------------
/**
 * @brief Encodes UTF-8 as modified UTF-7.
 *
 * @param[in] _sink  the destination.
 * @param[in] _utf8  the name, in UTF-8.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function or data-less text; D_IMAP_ERROR_ENCODING for
 *         invalid UTF-8 or a NUL; D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_mutf7_encode(struct d_pack_sink _sink,
                                      struct d_pack_text _utf8);
/**
 * @brief Decodes modified UTF-7 to UTF-8, strictly.
 *
 * @note rejected as D_IMAP_ERROR_ENCODING: characters outside printable
 *       US-ASCII; an unterminated, empty, or null-shifted ("-&") base64
 *       run; nonzero padding bits; an unpaired surrogate; a printable
 *       character or NUL that was base64-encoded.
 *
 * @param[in] _sink   the destination.
 * @param[in] _mutf7  the name, in modified UTF-7.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function or data-less text; D_IMAP_ERROR_ENCODING as
 *         noted; D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_mutf7_decode(struct d_pack_sink _sink,
                                      struct d_pack_text _mutf7);
/**
 * @brief Writes a UTF-8 mailbox name as a command argument.
 *
 * @note without D_IMAP_STRING_UTF8 the name is written in modified UTF-7,
 *       as an atom or quoted string, never a literal; with it, the name is
 *       written as d_imap_astring_write() would write it.
 *
 * @param[in]  _sink   the destination.
 * @param[in]  _name   the name, in UTF-8.
 * @param[in]  _flags  D_IMAP_STRING_* flags.
 * @param[out] _form   receives the form used; may be `NULL`, in which case a
 *                     name needing a literal is refused, not written.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function, data-less text, or a literal with no `_form`;
 *         D_IMAP_ERROR_ENCODING for invalid UTF-8 or a NUL;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_mailbox_write(struct d_pack_sink       _sink,
                                       struct d_pack_text       _name,
                                       uint32_t                 _flags,
                                       enum d_imap_string_form* _form);
// INBOX is the one mailbox name that compares case-insensitively; none
// allocate or fail
bool              d_imap_mailbox_is_inbox(struct d_pack_text _name);


//==============================================================================
// 10.  SEQUENCE SETS
//==============================================================================
// Message sets such as "1:4,7,9:*", naming sequence numbers or UIDs. "*" is
// the largest number in use, and "4:2" means the same as "2:4". The saved
// search result "$" (RFC 5182) is not represented.


// 10.1   Ranges
//------------------------------------------------------------------------------
// 10.1.1
// D_IMAP_SEQ_STAR
//   constant: the value that stands for "*". Real sequence numbers and UIDs
// start at 1, so 0 is free to carry it.
#define D_IMAP_SEQ_STAR 0u

// 10.1.2
// d_imap_seq_range
//   struct: one element of a set; `first` equal to `last` is a single number.
struct d_imap_seq_range
{
    uint32_t first;  // a number, or D_IMAP_SEQ_STAR
    uint32_t last;   // a number, or D_IMAP_SEQ_STAR
};

// 10.2   Sequence-set operations
//------------------------------------------------------------------------------
/**
 * @brief Writes ranges as a sequence set, in the order given.
 *
 * @param[in] _sink    the destination.
 * @param[in] _ranges  the ranges.
 * @param[in] _count   how many; at least one.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function, a `NULL` `_ranges`, or a `_count` of 0;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_seqset_write(struct d_pack_sink             _sink,
                                      const struct d_imap_seq_range* _ranges,
                                      size_t                         _count);
/**
 * @brief Parses a sequence set into ranges.
 *
 * @note a measuring call passes a `NULL` `_ranges` and a `_capacity` of 0.
 *       Leading zeros are accepted; the number 0 is not.
 *
 * @param[in]  _text      the sequence set.
 * @param[out] _ranges    receives up to `_capacity` ranges; may be `NULL`
 *                        when `_capacity` is 0.
 * @param[in]  _capacity  the room in `_ranges`.
 * @param[out] _count     receives the number of ranges in the set, even
 *                        when that exceeds `_capacity`.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a `NULL`
 *         `_count`, or a `NULL` `_ranges` with room; D_IMAP_ERROR_SYNTAX for
 *         a malformed set; D_IMAP_ERROR_RANGE for 0 or a number above
 *         D_IMAP_NUMBER_MAX; D_IMAP_ERROR_CAPACITY if the set has more ranges
 *         than `_capacity`.
 */
enum d_imap_error d_imap_seqset_parse(struct d_pack_text       _text,
                                      struct d_imap_seq_range* _ranges,
                                      size_t                   _capacity,
                                      size_t*                  _count);
// membership -- none allocate or fail. "*" resolves to `_largest`, the
// highest number in use; 0 and a NULL `_ranges` are in no set.
bool d_imap_seqset_contains(const struct d_imap_seq_range* _ranges,
                            size_t                         _count,
                            uint32_t                       _value,
                            uint32_t                       _largest);


//==============================================================================
// 11.  DATES
//==============================================================================
// SEARCH takes a date ("1-Feb-2026"); APPEND and INTERNALDATE carry a quoted
// date-time with a numeric zone ("\" 1-Feb-2026 09:05:07 -0500\""). Years
// are four digits, 0000 through 9999, in the proleptic Gregorian calendar.


// 11.1   Broken-down time
//------------------------------------------------------------------------------
// 11.1.1
// d_imap_datetime
//   struct: a local time and its offset from UTC. A small value, passed by
// value to the writers.
struct d_imap_datetime
{
    int32_t year;    // 0 .. 9999
    int32_t month;   // 1 .. 12
    int32_t day;     // 1 .. the month's length
    int32_t hour;    // 0 .. 23
    int32_t minute;  // 0 .. 59
    int32_t second;  // 0 .. 60, admitting a leap second
    int32_t zone;    // minutes east of UTC, -1439 .. 1439
};

// 11.2   Date operations
//------------------------------------------------------------------------------
/**
 * @brief Converts a Unix time to the local time of a given zone.
 *
 * @param[in]  _seconds   seconds since 1970-01-01T00:00:00Z.
 * @param[in]  _zone      the zone, in minutes east of UTC.
 * @param[out] _datetime  receives the local time.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_datetime` is
 *         `NULL`; D_IMAP_ERROR_RANGE if `_zone` is out of range or the year
 *         falls outside 0000 .. 9999.
 */
enum d_imap_error d_imap_datetime_from_unix(int64_t                 _seconds,
                                            int32_t                 _zone,
                                            struct d_imap_datetime* _datetime);
/**
 * @brief Converts a local time to Unix time.
 *
 * @note a leap second counts as the first second of the next minute.
 *
 * @param[in]  _datetime  the local time.
 * @param[out] _seconds   receives seconds since 1970-01-01T00:00:00Z.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_seconds` is
 *         `NULL`; D_IMAP_ERROR_RANGE if a field is out of range.
 */
enum d_imap_error d_imap_datetime_to_unix(struct d_imap_datetime _datetime,
                                          int64_t*               _seconds);
/**
 * @brief Writes the date part as a SEARCH date, "1-Feb-2026", unquoted.
 *
 * @param[in] _sink      the destination.
 * @param[in] _datetime  the date; time and zone are ignored.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function; D_IMAP_ERROR_RANGE for an invalid date;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_date_write(struct d_pack_sink     _sink,
                                    struct d_imap_datetime _datetime);
/**
 * @brief Writes a quoted date-time, "\" 1-Feb-2026 09:05:07 -0500\"".
 *
 * @param[in] _sink      the destination.
 * @param[in] _datetime  the time.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function; D_IMAP_ERROR_RANGE if a field is out of range;
 *         D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_datetime_write(struct d_pack_sink     _sink,
                                        struct d_imap_datetime _datetime);
/**
 * @brief Parses a date, quoted or not; the day may have one or two digits.
 *
 * @param[in]  _text      the date, "1-Feb-2026".
 * @param[out] _datetime  receives it, with time and zone zeroed; untouched
 *                        on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_datetime` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX for a malformed date;
 *         D_IMAP_ERROR_RANGE for an impossible one.
 */
enum d_imap_error d_imap_date_parse(struct d_pack_text      _text,
                                    struct d_imap_datetime* _datetime);
/**
 * @brief Parses a date-time, quoted or not; the day may be space-padded.
 *
 * @param[in]  _text      the date-time, " 1-Feb-2026 09:05:07 -0500".
 * @param[out] _datetime  receives it; untouched on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_datetime` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX for a malformed date-time;
 *         D_IMAP_ERROR_RANGE for an impossible one.
 */
enum d_imap_error d_imap_datetime_parse(struct d_pack_text      _text,
                                        struct d_imap_datetime* _datetime);


//==============================================================================
// 12.  RESPONSES
//==============================================================================
// A server line is tagged ("A0001 OK ..."), untagged ("* ..."), or a
// continuation request ("+ ..."). A status response carries a condition, an
// optional bracketed response code, and human-readable text; an untagged data
// response carries a keyword, perhaps a leading number, and data.


// 12.1   Conditions
//------------------------------------------------------------------------------
// 12.1.1
// d_imap_condition
//   enum: the condition of a status response. NONE marks a response that is
// not one: untagged data, or a continuation.
enum d_imap_condition
{
    D_IMAP_CONDITION_NONE    = 0,
    D_IMAP_CONDITION_OK      = 1,
    D_IMAP_CONDITION_NO      = 2,
    D_IMAP_CONDITION_BAD     = 3,
    D_IMAP_CONDITION_PREAUTH = 4,
    D_IMAP_CONDITION_BYE     = 5
};

// 12.1.2
// D_IMAP_CONDITION_COUNT
//   constant: the number of enumerators in d_imap_condition.
#define D_IMAP_CONDITION_COUNT 6

// 12.2   Response codes
//------------------------------------------------------------------------------
// 12.2.1
// d_imap_code
//   enum: the bracketed response codes this vocabulary names: RFC 9051
// section 7.1 (which absorbs RFC 3501, RFC 5530, and UIDPLUS), CONDSTORE and
// QRESYNC (RFC 7162), and the extensions commented. OTHER marks a code
// outside the vocabulary, whose atom the parsed response still carries.
enum d_imap_code
{
    D_IMAP_CODE_NONE                 = 0,   // no code
    D_IMAP_CODE_OTHER                = 1,   // not in this vocabulary
    D_IMAP_CODE_ALERT                = 2,
    D_IMAP_CODE_BADCHARSET           = 3,
    D_IMAP_CODE_CAPABILITY           = 4,
    D_IMAP_CODE_PARSE                = 5,
    D_IMAP_CODE_PERMANENTFLAGS       = 6,
    D_IMAP_CODE_READ_ONLY            = 7,
    D_IMAP_CODE_READ_WRITE           = 8,
    D_IMAP_CODE_TRYCREATE            = 9,
    D_IMAP_CODE_UIDNEXT              = 10,
    D_IMAP_CODE_UIDVALIDITY          = 11,
    D_IMAP_CODE_UNSEEN               = 12,
    D_IMAP_CODE_HASCHILDREN          = 13,
    D_IMAP_CODE_APPENDUID            = 14,  // RFC 4315
    D_IMAP_CODE_COPYUID              = 15,  // RFC 4315
    D_IMAP_CODE_UIDNOTSTICKY         = 16,  // RFC 4315
    D_IMAP_CODE_UNAVAILABLE          = 17,  // RFC 5530, through NONEXISTENT
    D_IMAP_CODE_AUTHENTICATIONFAILED = 18,
    D_IMAP_CODE_AUTHORIZATIONFAILED  = 19,
    D_IMAP_CODE_EXPIRED              = 20,
    D_IMAP_CODE_PRIVACYREQUIRED      = 21,
    D_IMAP_CODE_CONTACTADMIN         = 22,
    D_IMAP_CODE_NOPERM               = 23,
    D_IMAP_CODE_INUSE                = 24,
    D_IMAP_CODE_EXPUNGEISSUED        = 25,
    D_IMAP_CODE_CORRUPTION           = 26,
    D_IMAP_CODE_SERVERBUG            = 27,
    D_IMAP_CODE_CLIENTBUG            = 28,
    D_IMAP_CODE_CANNOT               = 29,
    D_IMAP_CODE_LIMIT                = 30,
    D_IMAP_CODE_OVERQUOTA            = 31,
    D_IMAP_CODE_ALREADYEXISTS        = 32,
    D_IMAP_CODE_NONEXISTENT          = 33,
    D_IMAP_CODE_HIGHESTMODSEQ        = 34,  // RFC 7162, through CLOSED
    D_IMAP_CODE_NOMODSEQ             = 35,
    D_IMAP_CODE_MODIFIED             = 36,
    D_IMAP_CODE_CLOSED               = 37,
    D_IMAP_CODE_UNKNOWN_CTE          = 38,  // RFC 3516
    D_IMAP_CODE_BADURL               = 39,  // RFC 4469
    D_IMAP_CODE_TOOBIG               = 40,  // RFC 4469, RFC 7889
    D_IMAP_CODE_COMPRESSIONACTIVE    = 41,  // RFC 4978
    D_IMAP_CODE_NOTSAVED             = 42,  // RFC 5182
    D_IMAP_CODE_USEATTR              = 43,  // RFC 6154
    D_IMAP_CODE_MAILBOXID            = 44   // RFC 8474
};

// 12.2.2
// D_IMAP_CODE_COUNT
//   constant: the number of enumerators in d_imap_code.
#define D_IMAP_CODE_COUNT 45

// 12.3   Condition and code queries
//------------------------------------------------------------------------------
// condition and code queries -- none allocate or fail. Names are the wire
// spellings ("READ-ONLY"), empty for NONE, OTHER, and unknown values; lookups
// match case-insensitively and fall back to NONE (conditions) or OTHER
// (codes). A condition maps to the error a command meeting it should report:
// NO, BAD, and BYE to their own codes, anything else to D_IMAP_ERROR_NONE.
const char*           d_imap_condition_name(enum d_imap_condition _condition);
enum d_imap_condition d_imap_condition_from_name(struct d_pack_text _name);
const char*           d_imap_code_name(enum d_imap_code _code);
enum d_imap_code      d_imap_code_from_name(struct d_pack_text _name);
enum d_imap_error     d_imap_error_from_condition(
                          enum d_imap_condition _condition);

// 12.4   Status responses
//------------------------------------------------------------------------------
// 12.4.1
// d_imap_response_kind
//   enum: the three shapes a server line takes.
enum d_imap_response_kind
{
    D_IMAP_RESPONSE_TAGGED       = 0,  // completes one command
    D_IMAP_RESPONSE_UNTAGGED     = 1,  // "*": status or data
    D_IMAP_RESPONSE_CONTINUATION = 2   // "+": the server awaits more
};

// 12.4.2
// d_imap_response
//   struct: the head of one server response, parsed. Every text field is a
// view into the parsed input, and empty when it does not apply. For a status
// response, `keyword` is the condition's word and `text` its human-readable
// text; for untagged data, `keyword` names the data ("EXISTS", "FETCH",
// "CAPABILITY") and `rest` holds everything after it, literals included, for
// the lexer; for a continuation, `text` holds the prompt or base64 challenge.
struct d_imap_response
{
    int32_t            kind;           // enum d_imap_response_kind
    int32_t            condition;      // enum d_imap_condition
    int32_t            code;           // enum d_imap_code
    int32_t            has_number;     // 1 if "* n KEYWORD", else 0
    uint32_t           number;         // that n
    struct d_pack_text tag;            // the tag of a tagged response
    struct d_pack_text keyword;        // the condition or data keyword
    struct d_pack_text code_name;      // the code's atom, as sent
    struct d_pack_text code_argument;  // the code's argument, up to "]"
    struct d_pack_text text;           // a status or continuation's text
    struct d_pack_text rest;           // untagged data after the keyword
};

// 12.5   Response parsing
//------------------------------------------------------------------------------
/**
 * @brief Parses the head of one server response.
 *
 * @note `_line` starts at the response and may run past its first line: a
 *       response carrying literals continues after each one, and `rest`
 *       extends to the end of `_line`, less one trailing CRLF. A bare LF is
 *       accepted where CRLF belongs. Tagged responses may be OK, NO, or BAD
 *       only.
 *
 * @param[in]  _line      the response.
 * @param[out] _response  receives the parse; untouched on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_response` is
 *         `NULL` or `_line` has a length but no data; D_IMAP_ERROR_SYNTAX for
 *         a malformed head; D_IMAP_ERROR_RANGE if the leading number exceeds
 *         D_IMAP_NUMBER_MAX.
 */
enum d_imap_error d_imap_response_parse(struct d_pack_text       _line,
                                        struct d_imap_response*  _response);


//==============================================================================
// 13.  LEXER
//==============================================================================
// Splits response data into tokens: atoms, numbers, NIL, quoted strings,
// literals, backslash flags, and punctuation. Spaces separate tokens and are
// not returned. Where IMAP's grammar depends on context, the caller says
// which context applies: brackets are punctuation in FETCH data
// ("BODY[TEXT]") but ordinary characters in a mailbox name ("[Gmail]/Sent").
// A lexer is a plain value; copying it saves a position to return to.


// 13.1   Tokens
//------------------------------------------------------------------------------
// 13.1.1
// d_imap_token_kind
//   enum: what a token is, and what its text holds.
enum d_imap_token_kind
{
    D_IMAP_TOKEN_END      = 0,   // the input is exhausted
    D_IMAP_TOKEN_CRLF     = 1,   // a line break
    D_IMAP_TOKEN_ATOM     = 2,   // text: the atom
    D_IMAP_TOKEN_NUMBER   = 3,   // text: the digits; number: their value
    D_IMAP_TOKEN_NIL      = 4,   // text: "NIL" as sent
    D_IMAP_TOKEN_QUOTED   = 5,   // text: the content, still escaped
    D_IMAP_TOKEN_LITERAL  = 6,   // text: the octets; number: their count
    D_IMAP_TOKEN_FLAG     = 7,   // text: "\" and the name, or "\*"
    D_IMAP_TOKEN_LPAREN   = 8,   // "("
    D_IMAP_TOKEN_RPAREN   = 9,   // ")"
    D_IMAP_TOKEN_LBRACKET = 10,  // "["
    D_IMAP_TOKEN_RBRACKET = 11,  // "]"
    D_IMAP_TOKEN_STAR     = 12   // "*"
};

// 13.1.2
// D_IMAP_TOKEN_ESCAPED
//   constant: token flag: a quoted token's content holds backslash escapes,
// so d_imap_string_decode() must unescape it.
#define D_IMAP_TOKEN_ESCAPED ((uint32_t)1 << 0)

// 13.1.3
// D_IMAP_TOKEN_NONSYNC
//   constant: token flag: a literal was announced non-synchronizing ("{n+}").
#define D_IMAP_TOKEN_NONSYNC ((uint32_t)1 << 1)

// 13.1.4
// D_IMAP_TOKEN_BINARY
//   constant: token flag: a literal was a literal8 ("~{n}", RFC 3516), whose
// octets may include NUL.
#define D_IMAP_TOKEN_BINARY  ((uint32_t)1 << 2)

// 13.1.5
// D_IMAP_LEX_ASTRING
//   constant: lexer flag: lex in astring context, where "[" and "]" are
// ordinary characters of an atom rather than punctuation.
#define D_IMAP_LEX_ASTRING   ((uint32_t)1 << 0)

// 13.1.6
// d_imap_token
//   struct: one token. Its text is a view into the lexer's input.
struct d_imap_token
{
    uint64_t           number;  // NUMBER: value; LITERAL: declared count
    int32_t            kind;    // enum d_imap_token_kind
    uint32_t           flags;   // D_IMAP_TOKEN_* bits
    struct d_pack_text text;    // see d_imap_token_kind
};

// 13.1.7
// d_imap_lexer
//   struct: a position in borrowed input.
struct d_imap_lexer
{
    const char* data;      // the input
    size_t      length;    // octets in the input
    size_t      position;  // octets consumed
};

// 13.2   Lexer operations
//------------------------------------------------------------------------------
/**
 * @brief Points a lexer at the start of some input.
 *
 * @param[out] _lexer  the lexer; `NULL` does nothing.
 * @param[in]  _input  the input, borrowed for the lexer's lifetime.
 */
void               d_imap_lexer_init(struct d_imap_lexer* _lexer,
                                     struct d_pack_text   _input);
/**
 * @brief Reads the next token.
 *
 * @note an all-digit atom whose value fits in 64 bits is a NUMBER, and "NIL"
 *       in any case is NIL. Octets above 0x7F are accepted inside atoms and
 *       quoted strings, which some servers send unquoted. On
 *       D_IMAP_ERROR_INCOMPLETE the position does not move and the token
 *       describes the partial item: for a literal, `number` is its declared
 *       count and `text` the octets present so far, which tells a reader
 *       how much more to fetch.
 *
 * @param[in,out] _lexer  the lexer.
 * @param[in]     _flags  D_IMAP_LEX_* flags for this token.
 * @param[out]    _token  receives the token.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if a pointer is
 *         `NULL`; D_IMAP_ERROR_SYNTAX for input that is no token;
 *         D_IMAP_ERROR_RANGE for a literal count above D_IMAP_NUMBER64_MAX;
 *         D_IMAP_ERROR_INCOMPLETE if the input ends inside a token.
 */
enum d_imap_error  d_imap_lexer_next(struct d_imap_lexer* _lexer,
                                     uint32_t             _flags,
                                     struct d_imap_token* _token);
// the unread input -- none allocate or fail; empty for a NULL lexer
struct d_pack_text d_imap_lexer_rest(const struct d_imap_lexer* _lexer);


//==============================================================================
// 14.  RESPONSE DATA
//==============================================================================
// Parsers for the untagged data every client meets, built on the lexer. Each
// takes the data after its keyword, as d_imap_response.rest holds it: after
// "LIST", after "SEARCH". FLAGS, STATUS, and CAPABILITY data parse through
// sections 6 and 8; the lexer serves the rest.


// 14.1   String values
//------------------------------------------------------------------------------
/**
 * @brief Writes the value of a string token: an atom's or number's text, a
 *        quoted string's unescaped content, or a literal's octets.
 *
 * @note NIL writes nothing and succeeds; the token's kind tells it apart
 *       from an empty string.
 *
 * @param[in] _sink   the destination.
 * @param[in] _token  an ATOM, NUMBER, NIL, QUOTED, or LITERAL token.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function, a `NULL` token, or another kind;
 *         D_IMAP_ERROR_SYNTAX for a bad escape; D_IMAP_ERROR_SINK if the sink
 *         refuses.
 */
enum d_imap_error d_imap_string_decode(struct d_pack_sink         _sink,
                                       const struct d_imap_token* _token);
/**
 * @brief Writes a mailbox-name token as UTF-8.
 *
 * @param[in] _sink   the destination.
 * @param[in] _token  an ATOM, NUMBER, QUOTED, or LITERAL token.
 * @param[in] _flags  D_IMAP_STRING_UTF8 when the session enabled
 *                    UTF8=ACCEPT, in which case the name is already UTF-8
 *                    and is validated rather than decoded from modified
 *                    UTF-7.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a sink without
 *         a write function, a `NULL` token, or another kind;
 *         D_IMAP_ERROR_SYNTAX for a bad escape; D_IMAP_ERROR_ENCODING for an
 *         invalid name; D_IMAP_ERROR_SINK if the sink refuses.
 */
enum d_imap_error d_imap_mailbox_decode(struct d_pack_sink         _sink,
                                        const struct d_imap_token* _token,
                                        uint32_t                   _flags);

// 14.2   Mailbox listings
//------------------------------------------------------------------------------
// 14.2.1
// d_imap_list_entry
//   struct: one LIST or LSUB response. The name stays encoded, since only the
// session knows whether it is modified UTF-7; d_imap_mailbox_decode()
// converts it.
struct d_imap_list_entry
{
    uint32_t            attributes;  // D_IMAP_MAILBOX_BIT of each attribute
    uint32_t            unknown;     // attributes outside the vocabulary
    int32_t             delimiter;   // hierarchy delimiter octet; -1 for NIL
    struct d_imap_token name;        // ATOM, NUMBER, QUOTED, or LITERAL
    struct d_pack_text  extended;    // LIST-EXTENDED data after the name
};

/**
 * @brief Parses the data of a LIST or LSUB response.
 *
 * @param[in]  _data   what follows the keyword: attributes, delimiter, name.
 * @param[out] _entry  receives the listing; untouched on failure.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT if `_entry` is
 *         `NULL`; D_IMAP_ERROR_SYNTAX for malformed data;
 *         D_IMAP_ERROR_INCOMPLETE if a literal name is not yet all present.
 */
enum d_imap_error d_imap_list_parse(struct d_pack_text        _data,
                                    struct d_imap_list_entry* _entry);

// 14.3   Search results
//------------------------------------------------------------------------------
/**
 * @brief Parses the data of a SEARCH response: numbers, and the trailing
 *        "(MODSEQ n)" CONDSTORE adds.
 *
 * @note ESEARCH (RFC 4731) answers in a different shape and is not parsed
 *       here. A measuring call passes a `NULL` `_ids` and a `_capacity` of 0.
 *
 * @param[in]  _data      what follows "SEARCH"; may be empty.
 * @param[out] _ids       receives up to `_capacity` numbers; may be `NULL`
 *                        when `_capacity` is 0.
 * @param[in]  _capacity  the room in `_ids`.
 * @param[out] _count     receives the number of results, even when that
 *                        exceeds `_capacity`.
 * @param[out] _modseq    receives the MODSEQ, or 0 without one; may be
 *                        `NULL`.
 * @return D_IMAP_ERROR_NONE; D_IMAP_ERROR_INVALID_ARGUMENT for a `NULL`
 *         `_count`, or a `NULL` `_ids` with room; D_IMAP_ERROR_SYNTAX for
 *         malformed data; D_IMAP_ERROR_RANGE for 0 or an oversized number;
 *         D_IMAP_ERROR_CAPACITY if there are more results than `_capacity`.
 */
enum d_imap_error d_imap_search_parse(struct d_pack_text _data,
                                      uint32_t*          _ids,
                                      size_t             _capacity,
                                      size_t*            _count,
                                      uint64_t*          _modseq);


D_EXTERN_C_END


#endif  // DJINTERP_NET_IMAP_IMAP_H
