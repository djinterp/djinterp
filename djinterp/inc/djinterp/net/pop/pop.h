/*******************************************************************************
* djinterp [net]                                                           pop.h
*
* The shared POP3 kernel: tier 0 of the POP modules.
*   Everything a POP3 client, server, proxy, or test double has in common,
* which is the protocol grammar itself, independent of how bytes move:
*     - the RFC 1939 state machine and command table, as data         [3]
*     - RFC 2449 extensions: response codes and CAPA parsing           [4]
*     - both directions of the line grammar, and the listing records   [5]
*     - a transport interface every derived module plugs into          [6]
*     - streaming dot-stuffing codecs for multi-line bodies            [7]
*     - a resumable, pipelining-safe input reader                      [8]
*     - the APOP digest, with an internal MD5                          [9]
*   NOTHING HERE OPENS A SOCKET OR ALLOCATES. A derived module supplies a
* d_pop_transport (socket, TLS session, curl handle, memory) and owns its
* storage; this file never learns which. That is what lets a client and a
* server be tested against each other over a memory transport and ship over
* a socket unchanged.
*   Both peers read the same tables. Whether a command is legal in a state,
* how many arguments it takes, and whether its reply carries a body are
* stated once, so the two ends of a session cannot drift apart.
*   Text inputs and outputs are d_pack_text spans from sink_common.h; output
* streams go to a d_pack_sink. Every declaration has C linkage, so one
* compiled pop.c serves C modules and any C++ face over them alike.
*   Build configuration comes from cfg_pop.h; environment detection for the
* derived modules from env_pop.h.
*
* path:      /inc/djinterp/net/pop/pop.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.25
*                                                            revised: 2026.09.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PROTOCOL CONSTANTS
    ------------------
    1.  Well-known ports
         1.  D_POP_PORT
         2.  D_POP_PORT_TLS
    2.  Wire limits
         1.  D_POP_COMMAND_MAX
         2.  D_POP_RESPONSE_MAX
         3.  D_POP_KEYWORD_MIN / D_POP_KEYWORD_MAX
         4.  D_POP_ARGS_MAX
         5.  D_POP_UID_MAX
         6.  D_POP_APOP_DIGEST_LENGTH
         7.  D_POP_AUTOLOGOUT_MIN_SECONDS
         8.  D_POP_LINE_CAPACITY
    3.  Wire tokens
         1.  D_POP_TOKEN_OK
         2.  D_POP_TOKEN_ERR
         3.  D_POP_TOKEN_CONTINUE
         4.  D_POP_TOKEN_CRLF
         5.  D_POP_TOKEN_TERMINATOR
2.  RESULT STATUS
    -------------
    1.  Status codes
         1.  d_pop_status
         2.  D_POP_STATUS_MECHANICAL_FLOOR
    2.  Status queries
3.  SESSION MODEL
    -------------
    1.  States
         1.  d_pop_state
         2.  D_POP_STATE_COUNT
         3.  D_POP_STATE_BIT
    2.  Indicators
         1.  d_pop_indicator
    3.  Commands
         1.  d_pop_command
         2.  D_POP_COMMAND_COUNT
         3.  d_pop_body_rule
         4.  d_pop_command_info
    4.  Command metadata and transitions
4.  EXTENSIONS
    ----------
    1.  Extended response codes
         1.  d_pop_resp_code
    2.  Capabilities
         1.  d_pop_capability
         2.  D_POP_VALUE_ABSENT / D_POP_EXPIRE_NEVER
         3.  d_pop_capabilities
    3.  Capability operations
    4.  Build features
         1.  d_pop_feature
5.  WIRE RECORDS
    ------------
    1.  Lines
         1.  d_pop_command_line
         2.  d_pop_response
    2.  Listings
         1.  d_pop_maildrop
         2.  d_pop_scan_listing
         3.  d_pop_uid_listing
    3.  Line codec
    4.  Listing codec
6.  TRANSPORT
    ---------
    1.  The transport interface
         1.  d_pop_transport_read_fn
         2.  d_pop_transport_write_fn
         3.  d_pop_transport
    2.  The memory transport
         1.  d_pop_memory_transport
    3.  Transport operations
7.  MULTI-LINE BODIES
    -----------------
    1.  Body decoding
         1.  d_pop_body_decoder
    2.  Body encoding
         1.  d_pop_body_encoder
    3.  Body operations
8.  READER
    ------
    1.  The buffered reader
         1.  d_pop_reader
    2.  Reader operations
9.  APOP
    ----
    1.  Digest operations
*/

#ifndef DJINTERP_NET_POP_POP_H
#define DJINTERP_NET_POP_POP_H 1

// std
#include <stddef.h>                          // size_t
#include <stdint.h>                          // int32_t, uint32_t, uint64_t
// djinterp
#include "../../c/djinterp.h"                // framework root
#include "../../c/util/sink_common.h"        // d_pack_text, d_pack_sink
#include "../../config/net/pop/cfg_pop.h"    // D_INTERNAL_POP_*

//   Every declaration below carries C linkage, so the one compiled pop.c
// serves both the C modules and any C++ face built over this kernel. Say so
// plainly rather than failing as a cascade of syntax errors further down.
#if !defined(D_EXTERN_C_BEGIN)
    #error "pop.h needs D_EXTERN_C_BEGIN; set D_CFG_DEFINE_EXTERN_C to 1"
#endif

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PROTOCOL CONSTANTS
//==============================================================================
// Numbers fixed by the RFCs: RFC 1939 (POP3), RFC 2449 (extension mechanism),
// RFC 2595 (STLS), RFC 3206 (SYS and AUTH codes), RFC 5034 (SASL AUTH), and
// RFC 6856 (UTF8 and LANG). Nothing in this section is configurable.


// 1.1    Well-known ports
//------------------------------------------------------------------------------
// 1.1.1
// D_POP_PORT
//   constant: the POP3 port, cleartext or upgraded by STLS.
#define D_POP_PORT                   110

// 1.1.2
// D_POP_PORT_TLS
//   constant: the POP3S port, TLS from the first byte.
#define D_POP_PORT_TLS               995

// 1.2    Wire limits
//------------------------------------------------------------------------------
// 1.2.1
// D_POP_COMMAND_MAX
//   constant: the longest command line, in octets including CRLF (RFC 2449
// section 4). The formatter never emits a longer one.
#define D_POP_COMMAND_MAX            255

// 1.2.2
// D_POP_RESPONSE_MAX
//   constant: the longest status line a server may send, in octets including
// CRLF (RFC 2449 section 4). The formatter never emits a longer one; the
// reader accepts up to D_POP_LINE_CAPACITY, because servers exceed it.
#define D_POP_RESPONSE_MAX           512

// 1.2.3
// D_POP_KEYWORD_MIN / D_POP_KEYWORD_MAX
//   constant: the length bounds of a command keyword (RFC 2449 section 3).
#define D_POP_KEYWORD_MIN            3
#define D_POP_KEYWORD_MAX            4

// 1.2.4
// D_POP_ARGS_MAX
//   constant: the most arguments any standard command takes (APOP, TOP, and
// AUTH take two).
#define D_POP_ARGS_MAX               2

// 1.2.5
// D_POP_UID_MAX
//   constant: the longest unique-id, in characters (RFC 1939 section 7).
#define D_POP_UID_MAX                70

// 1.2.6
// D_POP_APOP_DIGEST_LENGTH
//   constant: the length of an APOP digest in hex characters, excluding NUL.
#define D_POP_APOP_DIGEST_LENGTH     32

// 1.2.7
// D_POP_AUTOLOGOUT_MIN_SECONDS
//   constant: the shortest inactivity timer a server may use (RFC 1939
// section 3). A client should not assume a server waits any less.
#define D_POP_AUTOLOGOUT_MIN_SECONDS 600

// 1.2.8
// D_POP_LINE_CAPACITY
//   constant: the reader's line-buffer capacity, from cfg_pop.h.
#define D_POP_LINE_CAPACITY          D_INTERNAL_POP_LINE_MAX

// 1.3    Wire tokens
//------------------------------------------------------------------------------
// 1.3.1
// D_POP_TOKEN_OK
//   constant: the positive status indicator.
#define D_POP_TOKEN_OK               "+OK"

// 1.3.2
// D_POP_TOKEN_ERR
//   constant: the negative status indicator.
#define D_POP_TOKEN_ERR              "-ERR"

// 1.3.3
// D_POP_TOKEN_CONTINUE
//   constant: the SASL continuation indicator (RFC 5034 section 4).
#define D_POP_TOKEN_CONTINUE         "+"

// 1.3.4
// D_POP_TOKEN_CRLF
//   constant: the line terminator.
#define D_POP_TOKEN_CRLF             "\r\n"

// 1.3.5
// D_POP_TOKEN_TERMINATOR
//   constant: the line that ends a multi-line response.
#define D_POP_TOKEN_TERMINATOR       ".\r\n"


//==============================================================================
// 2.  RESULT STATUS
//==============================================================================
// Two kinds of failure, kept apart as in compress_common.h. A FORMAL failure
// says the exchange is not valid POP3 -- a malformed line, a command outside
// its state. A MECHANICAL failure says the exchange may be fine and the
// machinery ran short -- a small buffer, a transport that would block or
// closed. They occupy disjoint ranges, so a caller that only wants to know
// "may I retry?" tests the range rather than the enumerator.
//   A server's -ERR is NOT a failure here. It is a well-formed response and
// parses with D_POP_STATUS_OK; its meaning is the caller's business.


// 2.1    Status codes
//------------------------------------------------------------------------------
// 2.1.1
// d_pop_status
//   enum: the result of every fallible operation in this module. Values are
// pinned and must not be reordered.
enum d_pop_status
{
    D_POP_STATUS_OK                = 0,

    // formal: the exchange is not valid POP3
    D_POP_STATUS_INVALID_ARGUMENT  = 0x001,
    D_POP_STATUS_MALFORMED         = 0x002,
    D_POP_STATUS_UNKNOWN_COMMAND   = 0x003,
    D_POP_STATUS_WRONG_STATE       = 0x004,
    D_POP_STATUS_UNSUPPORTED       = 0x005,
    D_POP_STATUS_LINE_TOO_LONG     = 0x006,

    // mechanical: the machinery ran short
    D_POP_STATUS_BUFFER_TOO_SMALL  = 0x100,
    D_POP_STATUS_WOULD_BLOCK       = 0x101,
    D_POP_STATUS_TRANSPORT_ERROR   = 0x102,
    D_POP_STATUS_CONNECTION_CLOSED = 0x103,
    D_POP_STATUS_SINK_ERROR        = 0x104
};

// 2.1.2
// D_POP_STATUS_MECHANICAL_FLOOR
//   constant: the first mechanical status. Below it, the exchange itself was
// at fault; at or above it, the machinery was.
#define D_POP_STATUS_MECHANICAL_FLOOR 0x100

// 2.2    Status queries
//------------------------------------------------------------------------------
// status queries -- total over every value, never fail, never allocate
const char* d_pop_status_name(enum d_pop_status _status);
bool        d_pop_status_is_formal(enum d_pop_status _status);
bool        d_pop_status_is_mechanical(enum d_pop_status _status);


//==============================================================================
// 3.  SESSION MODEL
//==============================================================================
// The RFC 1939 state machine and the command vocabulary, described once as
// data. A client consults it to know what it may send and whether a body
// follows; a server consults the same table to know what it must refuse and
// whether it must send a body. Keeping one table is what stops the two sides
// of a loopback test from disagreeing about the protocol.


// 3.1    States
//------------------------------------------------------------------------------
// 3.1.1
// d_pop_state
//   enum: the session states of RFC 1939 section 3, plus CLOSED for "no
// session". A session enters AUTHORIZATION on a +OK greeting. UPDATE is
// terminal: the server commits deletions and closes the connection.
enum d_pop_state
{
    D_POP_STATE_CLOSED        = 0,
    D_POP_STATE_AUTHORIZATION = 1,
    D_POP_STATE_TRANSACTION   = 2,
    D_POP_STATE_UPDATE        = 3
};

// 3.1.2
// D_POP_STATE_COUNT
//   constant: the number of enumerators in d_pop_state.
#define D_POP_STATE_COUNT 4

// 3.1.3
// D_POP_STATE_BIT
//   macro: the bit for state `_s` in a d_pop_command_info state mask.
#define D_POP_STATE_BIT(_s) ((uint32_t)1u << (uint32_t)(_s))

// 3.2    Indicators
//------------------------------------------------------------------------------
// 3.2.1
// d_pop_indicator
//   enum: the leading token of a status line. CONTINUE is the "+ " line a
// server sends during a SASL exchange.
enum d_pop_indicator
{
    D_POP_INDICATOR_NONE     = 0,
    D_POP_INDICATOR_OK       = 1,
    D_POP_INDICATOR_ERR      = 2,
    D_POP_INDICATOR_CONTINUE = 3
};

// 3.3    Commands
//------------------------------------------------------------------------------
// 3.3.1
// d_pop_command
//   enum: the commands this module knows. UNKNOWN stands for any other
// keyword, which may still be sent and received as an extension. Values are
// pinned and index the metadata table in pop.c.
enum d_pop_command
{
    D_POP_COMMAND_UNKNOWN = 0,
    D_POP_COMMAND_USER    = 1,
    D_POP_COMMAND_PASS    = 2,
    D_POP_COMMAND_APOP    = 3,
    D_POP_COMMAND_AUTH    = 4,
    D_POP_COMMAND_STLS    = 5,
    D_POP_COMMAND_CAPA    = 6,
    D_POP_COMMAND_QUIT    = 7,
    D_POP_COMMAND_STAT    = 8,
    D_POP_COMMAND_LIST    = 9,
    D_POP_COMMAND_RETR    = 10,
    D_POP_COMMAND_DELE    = 11,
    D_POP_COMMAND_NOOP    = 12,
    D_POP_COMMAND_RSET    = 13,
    D_POP_COMMAND_TOP     = 14,
    D_POP_COMMAND_UIDL    = 15,
    D_POP_COMMAND_UTF8    = 16,
    D_POP_COMMAND_LANG    = 17
};

// 3.3.2
// D_POP_COMMAND_COUNT
//   constant: the number of enumerators in d_pop_command.
#define D_POP_COMMAND_COUNT 18

// 3.3.3
// d_pop_body_rule
//   enum: when a +OK reply to a command is followed by a multi-line body.
enum d_pop_body_rule
{
    D_POP_BODY_NEVER            = 0,
    D_POP_BODY_ALWAYS           = 1,
    D_POP_BODY_WITHOUT_ARGUMENT = 2
};

// 3.3.4
// d_pop_command_info
//   struct: static metadata for one command. `states` is a mask of
// D_POP_STATE_BIT values; `capability` is the d_pop_capability a server must
// advertise for the command to be usable, or 0 for a core command.
// `rest_of_line` marks a command whose last argument runs to the end of the
// line and may contain spaces (PASS, per RFC 1939 section 7).
struct d_pop_command_info
{
    const char* keyword;
    uint32_t    states;
    uint32_t    capability;
    uint8_t     min_args;
    uint8_t     max_args;
    uint8_t     body_rule;
    uint8_t     rest_of_line;
};

// 3.4    Command metadata and transitions
//------------------------------------------------------------------------------
// command metadata -- total over every value, never fail, never allocate.
// d_pop_command_info_of returns NULL only for D_POP_COMMAND_UNKNOWN and values
// outside the enumeration; d_pop_command_from_keyword matches case-
// insensitively and returns D_POP_COMMAND_UNKNOWN when nothing matches.
const struct d_pop_command_info*
                   d_pop_command_info_of(enum d_pop_command _command);
const char*        d_pop_command_keyword(enum d_pop_command _command);
enum d_pop_command d_pop_command_from_keyword(struct d_pack_text _keyword);
bool               d_pop_command_is_allowed(enum d_pop_command _command,
                                            enum d_pop_state   _state);
bool               d_pop_command_expects_body(enum d_pop_command _command,
                                              size_t             _arg_count);
const char*        d_pop_state_name(enum d_pop_state _state);

/**
 * @brief Computes the session state after one completed exchange.
 *
 * @note Both peers call this with the same arguments and so agree on the
 *       state without negotiating it. A client passes the indicator it read;
 *       a server passes the indicator it sent.
 *
 * @param[in] _state      the state in which the command was issued.
 * @param[in] _command    the command issued.
 * @param[in] _indicator  the indicator of the reply that completed it.
 * @return the next state. A successful PASS, APOP, or AUTH moves
 *         AUTHORIZATION to TRANSACTION; QUIT moves AUTHORIZATION to CLOSED and
 *         TRANSACTION to UPDATE; anything issued in UPDATE or CLOSED yields
 *         CLOSED. Every other exchange leaves the state unchanged.
 */
enum d_pop_state   d_pop_state_next(enum d_pop_state     _state,
                                    enum d_pop_command   _command,
                                    enum d_pop_indicator _indicator);


//==============================================================================
// 4.  EXTENSIONS
//==============================================================================
// The RFC 2449 extension mechanism: response codes a server may bracket onto a
// -ERR line, and the capability list a server returns to CAPA.


// 4.1    Extended response codes
//------------------------------------------------------------------------------
// 4.1.1
// d_pop_resp_code
//   enum: a recognized bracketed response code. UNKNOWN marks a well-formed
// code this module does not name; its text is still available on the
// response.
enum d_pop_resp_code
{
    D_POP_RESP_CODE_NONE        = 0,
    D_POP_RESP_CODE_UNKNOWN     = 1,
    D_POP_RESP_CODE_LOGIN_DELAY = 2,
    D_POP_RESP_CODE_IN_USE      = 3,
    D_POP_RESP_CODE_SYS_TEMP    = 4,
    D_POP_RESP_CODE_SYS_PERM    = 5,
    D_POP_RESP_CODE_AUTH        = 6,
    D_POP_RESP_CODE_UTF8        = 7
};

// 4.2    Capabilities
//------------------------------------------------------------------------------
// 4.2.1
// d_pop_capability
//   enum: one bit per capability keyword a CAPA response may list. Combined
// into d_pop_capabilities.flags.
enum d_pop_capability
{
    D_POP_CAPABILITY_TOP            = 0x0001,
    D_POP_CAPABILITY_USER           = 0x0002,
    D_POP_CAPABILITY_SASL           = 0x0004,
    D_POP_CAPABILITY_RESP_CODES     = 0x0008,
    D_POP_CAPABILITY_LOGIN_DELAY    = 0x0010,
    D_POP_CAPABILITY_PIPELINING     = 0x0020,
    D_POP_CAPABILITY_EXPIRE         = 0x0040,
    D_POP_CAPABILITY_UIDL           = 0x0080,
    D_POP_CAPABILITY_IMPLEMENTATION = 0x0100,
    D_POP_CAPABILITY_STLS           = 0x0200,
    D_POP_CAPABILITY_UTF8           = 0x0400,
    D_POP_CAPABILITY_LANG           = 0x0800,
    D_POP_CAPABILITY_AUTH_RESP_CODE = 0x1000
};

// 4.2.2
// D_POP_VALUE_ABSENT / D_POP_EXPIRE_NEVER
//   constant: sentinels for d_pop_capabilities' numeric fields -- the
// capability was not advertised, and EXPIRE NEVER, respectively.
#define D_POP_VALUE_ABSENT (-1)
#define D_POP_EXPIRE_NEVER (-2)

// 4.2.3
// d_pop_capabilities
//   struct: the parsed result of a CAPA response. The text members are copied
// in, NUL-terminated, so the struct outlives the lines it was parsed from.
// `login_delay` is in seconds and `expire` in days; either is
// D_POP_VALUE_ABSENT when not advertised. The *_per_user members are 1 when
// the server marked the value as varying by user.
struct d_pop_capabilities
{
    uint32_t flags;
    int32_t  login_delay;
    int32_t  login_delay_per_user;
    int32_t  expire;
    int32_t  expire_per_user;
    char     sasl_mechanisms[D_POP_LINE_CAPACITY];
    char     implementation[D_POP_LINE_CAPACITY];
};

// 4.3    Capability operations
//------------------------------------------------------------------------------
// capability queries -- never fail, never allocate. d_pop_capability_keyword
// returns NULL for a value that is not exactly one d_pop_capability bit.
void        d_pop_capabilities_init(struct d_pop_capabilities* _caps);
bool        d_pop_capabilities_has(const struct d_pop_capabilities* _caps,
                                   enum d_pop_capability            _cap);
const char* d_pop_capability_keyword(enum d_pop_capability _cap);

/**
 * @brief Folds one line of a CAPA response body into `_caps`.
 *
 * @note Unknown capability keywords are ignored, as RFC 2449 section 6
 *       requires, and succeed.
 *
 * @param[in,out] _caps  the capability set; initialize it first.
 * @param[in]     _line  one body line, without CRLF.
 * @pre `_caps` was initialized by d_pop_capabilities_init.
 * @return D_POP_STATUS_OK, D_POP_STATUS_INVALID_ARGUMENT for a NULL `_caps`,
 *         D_POP_STATUS_MALFORMED for a recognized keyword with invalid
 *         parameters, or D_POP_STATUS_BUFFER_TOO_SMALL when a text value does
 *         not fit its member (the flag is still set).
 */
enum d_pop_status
            d_pop_capabilities_parse_line(struct d_pop_capabilities* _caps,
                                          struct d_pack_text         _line);

// 4.4    Build features
//------------------------------------------------------------------------------
// 4.4.1
// d_pop_feature
//   enum: one bit per optional facility this build of the POP modules carries,
// as resolved by env_pop.h and cfg_pop.h. A server consults it before
// advertising STLS; a client before offering it.
enum d_pop_feature
{
    D_POP_FEATURE_TCP  = 0x01,
    D_POP_FEATURE_TLS  = 0x02,
    D_POP_FEATURE_CURL = 0x04,
    D_POP_FEATURE_SASL = 0x08,
    D_POP_FEATURE_APOP = 0x10
};

// build query -- a constant mask of d_pop_feature bits
uint32_t    d_pop_build_features(void);


//==============================================================================
// 5.  WIRE RECORDS
//==============================================================================
// Both directions of the line grammar. Parsers take a line WITHOUT its CRLF,
// as d_pop_reader_read_line delivers it, and fill a record whose text members
// BORROW from that line: they are valid exactly as long as the line is.
// Formatters follow the two-call buffer protocol of compress_common.h: pass
// `_out` NULL and `_capacity` 0 to measure; a buffer that is too small yields
// D_POP_STATUS_BUFFER_TOO_SMALL and still reports the size required. Output is
// not NUL-terminated; `*_out_size` is the exact wire length.


// 5.1    Lines
//------------------------------------------------------------------------------
// 5.1.1
// d_pop_command_line
//   struct: one command. `keyword` is the text as sent, and is what the
// formatter writes when `command` is D_POP_COMMAND_UNKNOWN; for a known
// command the canonical keyword is written and `keyword` is ignored.
struct d_pop_command_line
{
    enum d_pop_command command;
    struct d_pack_text keyword;
    struct d_pack_text args[D_POP_ARGS_MAX];
    size_t             arg_count;
};

// 5.1.2
// d_pop_response
//   struct: one status line. `code` and `code_text` describe a bracketed
// response code, which is recognized on -ERR lines only; `code_text` excludes
// the brackets. `text` is everything after the indicator (and code), with the
// separating space removed.
struct d_pop_response
{
    enum d_pop_indicator indicator;
    enum d_pop_resp_code code;
    struct d_pack_text   code_text;
    struct d_pack_text   text;
};

// 5.2    Listings
//------------------------------------------------------------------------------
// 5.2.1
// d_pop_maildrop
//   struct: the reply to STAT -- message count and total size in octets.
struct d_pop_maildrop
{
    uint32_t count;
    uint64_t octets;
};

// 5.2.2
// d_pop_scan_listing
//   struct: one LIST entry -- a message number and its size in octets.
struct d_pop_scan_listing
{
    uint32_t number;
    uint64_t octets;
};

// 5.2.3
// d_pop_uid_listing
//   struct: one UIDL entry. `uid` is copied in and NUL-terminated.
struct d_pop_uid_listing
{
    uint32_t number;
    char     uid[D_POP_UID_MAX + 1];
};

// 5.3    Line codec
//------------------------------------------------------------------------------
/**
 * @brief Parses one command line, as a server receives it.
 *
 * @param[in]  _line  the line, without CRLF.
 * @param[out] _out   the parsed command; its texts borrow from `_line`.
 * @return D_POP_STATUS_OK; D_POP_STATUS_UNKNOWN_COMMAND for a well-formed
 *         keyword this module does not know, with `keyword` and up to
 *         D_POP_ARGS_MAX arguments still filled; D_POP_STATUS_MALFORMED for a
 *         bad keyword or the wrong number of arguments; or
 *         D_POP_STATUS_INVALID_ARGUMENT for a NULL `_out`.
 */
enum d_pop_status d_pop_command_parse(struct d_pack_text         _line,
                                      struct d_pop_command_line* _out);

/**
 * @brief Formats one command line, CRLF included, as a client sends it.
 *
 * @param[in]  _command   the command; see d_pop_command_line.
 * @param[out] _out       the destination, or NULL to measure.
 * @param[in]  _capacity  the size of `_out` in bytes.
 * @param[out] _out_size  receives the wire length.
 * @return D_POP_STATUS_OK; D_POP_STATUS_MALFORMED for an invalid keyword, an
 *         argument count outside the command's range, or an argument that is
 *         empty or holds CR, LF, NUL, or (except a rest-of-line argument)
 *         SP; D_POP_STATUS_LINE_TOO_LONG past D_POP_COMMAND_MAX;
 *         D_POP_STATUS_BUFFER_TOO_SMALL; or D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_command_format(
                      const struct d_pop_command_line* _command,
                      char*                            _out,
                      size_t                           _capacity,
                      size_t*                          _out_size);

/**
 * @brief Parses one status line, as a client receives it.
 *
 * @note Indicators are matched case-insensitively. A bracketed code is
 *       recognized only on -ERR lines; on +OK a leading `[` is ordinary text.
 *
 * @param[in]  _line  the line, without CRLF.
 * @param[out] _out   the parsed response; its texts borrow from `_line`.
 * @return D_POP_STATUS_OK (for -ERR as much as +OK),
 *         D_POP_STATUS_MALFORMED when no indicator leads the line, or
 *         D_POP_STATUS_INVALID_ARGUMENT for a NULL `_out`.
 */
enum d_pop_status d_pop_response_parse(struct d_pack_text     _line,
                                       struct d_pop_response* _out);

/**
 * @brief Formats one status line, CRLF included, as a server sends it.
 *
 * @note A code is written when `code_text` is non-empty, or else when `code`
 *       names a known code; only a -ERR line may carry one.
 *
 * @param[in]  _response  the response to write.
 * @param[out] _out       the destination, or NULL to measure.
 * @param[in]  _capacity  the size of `_out` in bytes.
 * @param[out] _out_size  receives the wire length.
 * @return D_POP_STATUS_OK; D_POP_STATUS_MALFORMED for indicator NONE, a code
 *         on a non-ERR line, or text holding CR, LF, or NUL;
 *         D_POP_STATUS_LINE_TOO_LONG past D_POP_RESPONSE_MAX;
 *         D_POP_STATUS_BUFFER_TOO_SMALL; or D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_response_format(
                      const struct d_pop_response* _response,
                      char*                        _out,
                      size_t                       _capacity,
                      size_t*                      _out_size);

// response-code names -- total, never fail. _name returns NULL for NONE and
// UNKNOWN; _from_text matches case-insensitively and yields UNKNOWN for an
// unrecognized non-empty code and NONE for empty text.
const char*          d_pop_resp_code_name(enum d_pop_resp_code _code);
enum d_pop_resp_code d_pop_resp_code_from_text(struct d_pack_text _text);

// 5.4    Listing codec
//------------------------------------------------------------------------------
/**
 * @brief Parses a decimal field as it appears on the wire.
 *
 * @param[in]  _text     the digits; no sign, no whitespace.
 * @param[in]  _maximum  the largest acceptable value.
 * @param[out] _out      receives the value.
 * @return D_POP_STATUS_OK, D_POP_STATUS_MALFORMED for an empty field, a
 *         non-digit, or a value above `_maximum`, or
 *         D_POP_STATUS_INVALID_ARGUMENT for a NULL `_out`.
 */
enum d_pop_status d_pop_parse_decimal(struct d_pack_text _text,
                                      uint64_t           _maximum,
                                      uint64_t*          _out);

// listing parsers -- each takes the text after "+OK " (or one body line) and
// returns D_POP_STATUS_OK, D_POP_STATUS_MALFORMED, or
// D_POP_STATUS_INVALID_ARGUMENT for a NULL `_out`. A message number must be at
// least 1. Trailing text after the required fields is permitted and ignored,
// as RFC 1939 allows servers to append information.
enum d_pop_status d_pop_maildrop_parse(struct d_pack_text     _text,
                                       struct d_pop_maildrop* _out);
enum d_pop_status d_pop_scan_listing_parse(struct d_pack_text         _text,
                                           struct d_pop_scan_listing* _out);
enum d_pop_status d_pop_uid_listing_parse(struct d_pack_text        _text,
                                          struct d_pop_uid_listing* _out);

// listing formatters -- write the fields without CRLF, for use as +OK text or
// as a body line, under the two-call protocol. They fail with
// D_POP_STATUS_MALFORMED for a message number of 0 or an invalid uid,
// D_POP_STATUS_BUFFER_TOO_SMALL, or D_POP_STATUS_INVALID_ARGUMENT.
enum d_pop_status d_pop_maildrop_format(
                      const struct d_pop_maildrop* _maildrop,
                      char*                        _out,
                      size_t                       _capacity,
                      size_t*                      _out_size);
enum d_pop_status d_pop_scan_listing_format(
                      const struct d_pop_scan_listing* _listing,
                      char*                            _out,
                      size_t                           _capacity,
                      size_t*                          _out_size);
enum d_pop_status d_pop_uid_listing_format(
                      const struct d_pop_uid_listing* _listing,
                      char*                           _out,
                      size_t                          _capacity,
                      size_t*                         _out_size);

// uid validation -- 1 to D_POP_UID_MAX characters, each 0x21 to 0x7E
bool              d_pop_uid_is_valid(struct d_pack_text _uid);


//==============================================================================
// 6.  TRANSPORT
//==============================================================================
// The byte stream a session runs over, as two callbacks and a context. This is
// the seam every derived module plugs into: a socket module wraps a
// descriptor, a TLS module wraps a session, a test wraps memory. Nothing above
// this section knows which.
//   A transport may be blocking or non-blocking. A non-blocking one returns
// D_POP_STATUS_WOULD_BLOCK, and every consumer in this module keeps enough
// state to be called again later and resume where it stopped.


// 6.1    The transport interface
//------------------------------------------------------------------------------
// 6.1.1
// d_pop_transport_read_fn
//   function pointer: reads up to `_capacity` bytes into `_buffer`, storing
// the count in `*_out_read`. Returns D_POP_STATUS_OK with a count of at least
// 1, D_POP_STATUS_CONNECTION_CLOSED at orderly end of stream,
// D_POP_STATUS_WOULD_BLOCK, or D_POP_STATUS_TRANSPORT_ERROR.
typedef enum d_pop_status (*d_pop_transport_read_fn)(void*   _context,
                                                     void*   _buffer,
                                                     size_t  _capacity,
                                                     size_t* _out_read);

// 6.1.2
// d_pop_transport_write_fn
//   function pointer: writes up to `_size` bytes from `_data`, storing the
// count in `*_out_written`. A short write is permitted. Returns the same
// statuses as the read callback.
typedef enum d_pop_status (*d_pop_transport_write_fn)(void*       _context,
                                                      const void* _data,
                                                      size_t      _size,
                                                      size_t*     _out_written);

// 6.1.3
// d_pop_transport
//   struct: a transport and its context. Passed by value; holds no ownership
// of `context`.
struct d_pop_transport
{
    d_pop_transport_read_fn  read;
    d_pop_transport_write_fn write;
    void*                    context;
};

// 6.2    The memory transport
//------------------------------------------------------------------------------
// 6.2.1
// d_pop_memory_transport
//   struct: the context of a transport that reads from a fixed buffer and
// writes to a sink. `chunk`, when non-zero, caps every read, which is how a
// test proves a consumer survives arbitrary fragmentation. Reads past the end
// report CONNECTION_CLOSED, or WOULD_BLOCK when `block_at_end` is non-zero.
struct d_pop_memory_transport
{
    const unsigned char* input;
    size_t               input_size;
    size_t               input_offset;
    size_t               chunk;
    int32_t              block_at_end;
    struct d_pack_sink   output;
};

// 6.3    Transport operations
//------------------------------------------------------------------------------
// memory transport -- borrows `_input` and `_output`; neither allocates
void                   d_pop_memory_transport_init(
                           struct d_pop_memory_transport* _memory,
                           const void*                    _input,
                           size_t                         _input_size,
                           struct d_pack_sink             _output);
struct d_pop_transport d_pop_transport_from_memory(
                           struct d_pop_memory_transport* _memory);

/**
 * @brief Writes all of `_data`, looping over short writes.
 *
 * @param[in]  _transport    the transport.
 * @param[in]  _data         the bytes to send.
 * @param[in]  _size         how many.
 * @param[out] _out_written  receives the count written, which on a
 *                           non-OK status says where to resume; may be NULL.
 * @return D_POP_STATUS_OK once every byte is written, or the first non-OK
 *         status from the transport, or D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status      d_pop_transport_write_all(
                           struct d_pop_transport _transport,
                           const void*            _data,
                           size_t                 _size,
                           size_t*                _out_written);

// send helpers -- format into a stack buffer, then write_all. They return any
// status of the corresponding formatter or of d_pop_transport_write_all. On
// WOULD_BLOCK the line may be partly sent; a non-blocking caller formats and
// writes itself instead.
enum d_pop_status      d_pop_send_command(
                           struct d_pop_transport           _transport,
                           const struct d_pop_command_line* _command);
enum d_pop_status      d_pop_send_response(
                           struct d_pop_transport       _transport,
                           const struct d_pop_response* _response);


//==============================================================================
// 7.  MULTI-LINE BODIES
//==============================================================================
// RFC 1939 section 3 frames a multi-line body by "byte-stuffing": a line that
// begins with "." gains a second ".", and the body ends with a line holding a
// single ".". Both directions are incremental state machines over arbitrary
// chunks, so a body of any size streams through a sink without buffering and
// a line of any length is never a problem.
//   Decoded output keeps each line's CRLF and omits the terminating ".CRLF".
// A sink whose `write` is NULL discards, which is how a caller skips a body.


// 7.1    Body decoding
//------------------------------------------------------------------------------
// 7.1.1
// d_pop_body_decoder
//   struct: the state of one body being un-stuffed. `strict` defaults from
// cfg_pop.h: when 0, a bare LF also ends a line. `octets` counts the bytes
// emitted, for comparison against a scan listing.
struct d_pop_body_decoder
{
    int32_t  state;
    int32_t  strict;
    uint64_t octets;
};

// 7.2    Body encoding
//------------------------------------------------------------------------------
// 7.2.1
// d_pop_body_encoder
//   struct: the state of one body being stuffed. `normalize`, 1 by default,
// rewrites a bare LF as CRLF, since stored mail often uses LF alone and the
// wire requires CRLF.
struct d_pop_body_encoder
{
    int32_t  at_line_start;
    int32_t  after_cr;
    int32_t  normalize;
    int32_t  finished;
    uint64_t octets;
};

// 7.3    Body operations
//------------------------------------------------------------------------------
// initialization -- never fail
void d_pop_body_decoder_init(struct d_pop_body_decoder* _decoder);
bool d_pop_body_decoder_is_done(const struct d_pop_body_decoder* _decoder);
void d_pop_body_encoder_init(struct d_pop_body_encoder* _encoder);

/**
 * @brief Un-stuffs one chunk of a body into `_sink`.
 *
 * @note Decoding stops exactly after the terminator. Bytes beyond it belong to
 *       the next response (PIPELINING) and are left unconsumed.
 *
 * @param[in,out] _decoder   the decoder state.
 * @param[in]     _data      the chunk.
 * @param[in]     _size      its size.
 * @param[in]     _sink      where content goes; a NULL `write` discards.
 * @param[out]    _consumed  receives how many bytes of `_data` were used.
 * @return D_POP_STATUS_OK (test d_pop_body_decoder_is_done for completion),
 *         D_POP_STATUS_SINK_ERROR if the sink refused bytes, or
 *         D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_body_decode(struct d_pop_body_decoder* _decoder,
                                    const void*                _data,
                                    size_t                     _size,
                                    struct d_pack_sink         _sink,
                                    size_t*                    _consumed);

/**
 * @brief Stuffs one chunk of a body into `_sink`.
 *
 * @param[in,out] _encoder  the encoder state.
 * @param[in]     _data     the content.
 * @param[in]     _size     its size.
 * @param[in]     _sink     the destination.
 * @pre d_pop_body_encode_finish has not been called on `_encoder`.
 * @return D_POP_STATUS_OK, D_POP_STATUS_SINK_ERROR, or
 *         D_POP_STATUS_INVALID_ARGUMENT (including after finish).
 */
enum d_pop_status d_pop_body_encode(struct d_pop_body_encoder* _encoder,
                                    const void*                _data,
                                    size_t                     _size,
                                    struct d_pack_sink         _sink);

/**
 * @brief Ends a body: completes a trailing partial line, then writes the
 *        terminator.
 *
 * @param[in,out] _encoder  the encoder state.
 * @param[in]     _sink     the destination.
 * @post `_encoder` accepts no further content.
 * @return D_POP_STATUS_OK, D_POP_STATUS_SINK_ERROR, or
 *         D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_body_encode_finish(struct d_pop_body_encoder* _encoder,
                                           struct d_pack_sink         _sink);


//==============================================================================
// 8.  READER
//==============================================================================
// The single input buffer of a session. Status lines and bodies are read
// through the same reader because they share the stream: a +OK line and the
// start of its body typically arrive in one read, and with PIPELINING the
// next response may follow the body in the same read. Whatever is read and
// not yet consumed stays buffered for the next call.


// 8.1    The buffered reader
//------------------------------------------------------------------------------
// 8.1.1
// d_pop_reader
//   struct: an input buffer over a transport. `strict` defaults from
// cfg_pop.h: when 1, a status line ended by a bare LF is D_POP_STATUS_MALFORMED.
struct d_pop_reader
{
    size_t  start;
    size_t  end;
    size_t  scan;
    int32_t strict;
    int32_t discarding;
    char    buffer[D_POP_LINE_CAPACITY];
};

// 8.2    Reader operations
//------------------------------------------------------------------------------
// initialization and inspection -- never fail
void              d_pop_reader_init(struct d_pop_reader* _reader);
size_t            d_pop_reader_buffered(const struct d_pop_reader* _reader);

/**
 * @brief Reads one line, pulling from the transport as needed.
 *
 * @param[in,out] _reader     the reader.
 * @param[in]     _transport  the transport to pull from.
 * @param[out]    _out_line   receives the line without its line ending.
 * @post `*_out_line` borrows the reader's buffer and is invalidated by the
 *       next call on `_reader`.
 * @return D_POP_STATUS_OK; D_POP_STATUS_LINE_TOO_LONG when a line fills the
 *         buffer (the rest of it is discarded, and the next call resumes at
 *         the following line); D_POP_STATUS_MALFORMED for a bare LF in strict
 *         mode (the line is consumed); any non-OK transport status, after
 *         which the call may be repeated; or D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_reader_read_line(struct d_pop_reader*   _reader,
                                         struct d_pop_transport _transport,
                                         struct d_pack_text*    _out_line);

/**
 * @brief Reads one multi-line body into `_sink`, through its terminator.
 *
 * @param[in,out] _reader     the reader; buffered bytes are decoded first.
 * @param[in]     _transport  the transport to pull from.
 * @param[in,out] _decoder    the body state; initialize it per body.
 * @param[in]     _sink       where content goes; a NULL `write` discards.
 * @post On D_POP_STATUS_OK the terminator is consumed and bytes after it
 *       remain buffered in `_reader`.
 * @return D_POP_STATUS_OK once the body is complete; any non-OK transport
 *         status, after which the call may be repeated with the same
 *         decoder; D_POP_STATUS_SINK_ERROR; or
 *         D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_reader_read_body(struct d_pop_reader*       _reader,
                                         struct d_pop_transport     _transport,
                                         struct d_pop_body_decoder* _decoder,
                                         struct d_pack_sink         _sink);


//==============================================================================
// 9.  APOP
//==============================================================================
// RFC 1939 section 7: the server greets with a timestamp, and the client
// proves knowledge of a shared secret by sending MD5(timestamp || secret) in
// hex. APOP is shared by both sides -- a client computes the digest, a server
// computes it again to compare -- so it lives here. MD5 is implemented
// internally; no crypto library is needed. It is also broken: APOP resists
// passive replay and nothing more, and should run inside TLS where possible.


// 9.1    Digest operations
//------------------------------------------------------------------------------
/**
 * @brief Finds the APOP timestamp in a greeting's text.
 *
 * @param[in]  _greeting       the text of the +OK greeting.
 * @param[out] _out_timestamp  receives the timestamp, angle brackets
 *                             included; it borrows from `_greeting`.
 * @return D_POP_STATUS_OK, D_POP_STATUS_UNSUPPORTED when the greeting holds
 *         no `<...@...>` timestamp (the server does not offer APOP), or
 *         D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_apop_timestamp(struct d_pack_text  _greeting,
                                       struct d_pack_text* _out_timestamp);

/**
 * @brief Computes the APOP digest as lowercase hex.
 *
 * @param[in]  _timestamp  the timestamp, angle brackets included.
 * @param[in]  _secret     the shared secret.
 * @param[out] _out        receives D_POP_APOP_DIGEST_LENGTH hex characters
 *                         and a NUL.
 * @return D_POP_STATUS_OK, D_POP_STATUS_UNSUPPORTED when D_CFG_POP_APOP is
 *         0, or D_POP_STATUS_INVALID_ARGUMENT.
 */
enum d_pop_status d_pop_apop_digest(struct d_pack_text _timestamp,
                                    struct d_pack_text _secret,
                                    char _out[D_POP_APOP_DIGEST_LENGTH + 1]);

/**
 * @brief Checks a client's APOP digest, as a server does.
 *
 * @note The comparison takes the same time wherever the digests differ, and
 *       accepts hex in either case.
 *
 * @param[in] _timestamp  the timestamp the server issued.
 * @param[in] _secret     the shared secret.
 * @param[in] _digest     the digest the client sent.
 * @return `true` when `_digest` matches; `false` on mismatch, on malformed
 *         input, or when APOP is compiled out.
 */
bool              d_pop_apop_verify(struct d_pack_text _timestamp,
                                    struct d_pack_text _secret,
                                    struct d_pack_text _digest);


D_EXTERN_C_END


#endif  // DJINTERP_NET_POP_POP_H
