/*******************************************************************************
* djinterp [net]                                                   smtp_common.h
*
* djinterp SMTP protocol vocabulary.
*   The transport-independent half of the SMTP kernel: the ports and limits of
* RFC 5321, the error codes every SMTP module reports through, reply parsing
* (multiline replies and RFC 3463 enhanced status codes) and reply formatting,
* EHLO capability discovery, mailbox and domain validation, command parsing,
* base64 (RFC 4648), and the message-data transparency procedure of RFC 5321
* section 4.5.2.
*   Nothing here performs I/O or allocates. Every function works on
* caller-owned memory, which is what lets one implementation serve the client,
* the server, and the C++ facade in net/smtp.hpp.
*
*
* path:      /inc/djinterp/net/smtp/smtp_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Ports
    2.  Protocol limits
    3.  Kernel limits
2.  ERRORS AND BUFFERS
    ------------------
    1.  Error codes
         1.  d_smtp_error
    2.  Buffers
         1.  d_smtp_buffer
3.  REPLIES
    -------
    1.  Reply types
         1.  d_smtp_status
         2.  d_smtp_reply
    2.  Reply parsing
    3.  Reply codes
    4.  Reply formatting
4.  CAPABILITIES
    ------------
    1.  Capability types
         1.  d_smtp_extension
         2.  d_smtp_auth
         3.  d_smtp_capabilities
    2.  Capability discovery
5.  ADDRESSES AND COMMANDS
    ----------------------
    1.  Validation
    2.  Command types
         1.  d_smtp_verb
         2.  d_smtp_command
         3.  d_smtp_path
         4.  d_smtp_parameter
    3.  Command parsing
6.  ENCODINGS
    ---------
    1.  Base64
    2.  Message data
         1.  d_smtp_data_encoder
*/

#ifndef DJINTERP_NET_SMTP_SMTP_COMMON_H
#define DJINTERP_NET_SMTP_SMTP_COMMON_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================
// Numbers the RFCs fix, and the sizes of the kernel's own fixed buffers.


// 1.1    Ports
//------------------------------------------------------------------------------
// D_SMTP_PORT
//   constant: the relay port, for server-to-server transfer (RFC 5321).
#define D_SMTP_PORT                 25u

// D_SMTP_PORT_SUBMISSION
//   constant: the message-submission port, upgraded with STARTTLS (RFC 6409).
#define D_SMTP_PORT_SUBMISSION      587u

// D_SMTP_PORT_SUBMISSIONS
//   constant: message submission over implicit TLS (RFC 8314).
#define D_SMTP_PORT_SUBMISSIONS     465u

// 1.2    Protocol limits
//------------------------------------------------------------------------------
// D_SMTP_COMMAND_LINE_MAX
//   constant: the longest command line, CRLF included, that every server must
// accept (RFC 5321 section 4.5.3.1.4).
#define D_SMTP_COMMAND_LINE_MAX     512u

// D_SMTP_TEXT_LINE_MAX
//   constant: the longest line of message data, CRLF included (RFC 5321
// section 4.5.3.1.6).
#define D_SMTP_TEXT_LINE_MAX        1000u

// D_SMTP_LOCAL_PART_MAX
//   constant: the longest local-part of a mailbox (section 4.5.3.1.1).
#define D_SMTP_LOCAL_PART_MAX       64u

// D_SMTP_DOMAIN_MAX
//   constant: the longest domain name or address literal (4.5.3.1.2).
#define D_SMTP_DOMAIN_MAX           255u

// D_SMTP_PATH_MAX
//   constant: the longest path, angle brackets included (4.5.3.1.3). A mailbox
// is therefore at most D_SMTP_PATH_MAX - 2 bytes.
#define D_SMTP_PATH_MAX             256u

// 1.3    Kernel limits
//------------------------------------------------------------------------------
// D_SMTP_REPLY_TEXT_MAX
//   constant: bytes of reply text a d_smtp_reply keeps, NUL included. Longer
// replies keep their code and status and are marked truncated.
#define D_SMTP_REPLY_TEXT_MAX       4096u

// D_SMTP_LINE_MAX
//   constant: the longest line the kernel reads intact, NUL included. It is
// deliberately above the RFC minima: AUTH exchanges may reach 12288 bytes
// under RFC 4954, and lenient peers exceed the 512-byte reply limit. Longer
// lines are reported as D_SMTP_ERROR_TOO_LONG without losing stream sync.
#define D_SMTP_LINE_MAX             4096u


//==============================================================================
// 2.  ERRORS AND BUFFERS
//==============================================================================
// The error vocabulary shared by every SMTP module, and the caller-owned
// buffer the kernel writes into.


// 2.1    Error codes
//------------------------------------------------------------------------------
// 2.1.1
// d_smtp_error
//   enum: the outcome of every SMTP kernel operation. D_SMTP_OK is success.
// D_SMTP_ERROR_REJECTED means the peer answered with a negative reply, which
// the session keeps for inspection; every other value is local or transport
// failure.
enum d_smtp_error
{
    D_SMTP_OK = 0,
    D_SMTP_ERROR_INVALID,      // an argument is missing or malformed
    D_SMTP_ERROR_STATE,        // the call is out of order for the session
    D_SMTP_ERROR_NO_MEMORY,    // an allocation failed
    D_SMTP_ERROR_RESOLVE,      // the host name did not resolve
    D_SMTP_ERROR_CONNECT,      // no address accepted the connection
    D_SMTP_ERROR_TIMEOUT,      // an I/O deadline passed
    D_SMTP_ERROR_IO,           // the transport failed
    D_SMTP_ERROR_CLOSED,       // the peer closed the connection
    D_SMTP_ERROR_TLS,          // the handshake or certificate check failed
    D_SMTP_ERROR_PROTOCOL,     // the peer broke the protocol
    D_SMTP_ERROR_TOO_LONG,     // a line, reply or message exceeded its limit
    D_SMTP_ERROR_REJECTED,     // the peer answered with a negative reply
    D_SMTP_ERROR_UNSUPPORTED,  // not offered by the peer, or not built in
    D_SMTP_ERROR_AUTH,         // the peer refused the credentials
    D_SMTP_ERROR_INSECURE      // credentials would have crossed in the clear
};

// d_smtp_error_string -- a short label for an error; O(1), never NULL
const char*       d_smtp_error_string(enum d_smtp_error _error);

// 2.2    Buffers
//------------------------------------------------------------------------------
// 2.2.1
// d_smtp_buffer
//   struct: a caller-owned byte region the kernel appends into. One byte is
// always held back for a terminating NUL, so `length` stays below `capacity`
// and the contents are a C string as well as a counted span.
struct d_smtp_buffer
{
    char*  data;      // caller-owned storage
    size_t capacity;  // bytes of storage
    size_t length;    // bytes in use, excluding the NUL
};

// d_smtp_buffer_init -- points a buffer at `_capacity` bytes of `_data` and
// empties it; a NULL buffer is ignored and NULL data gives a 0-byte buffer
void              d_smtp_buffer_init(struct d_smtp_buffer* _buffer,
                                     char*                 _data,
                                     size_t                _capacity);

/**
 * @brief Appends bytes, keeping the contents NUL-terminated.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _data    the bytes; may be NULL when `_length` is 0.
 * @param[in]     _length  number of bytes to append.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL buffer or data; or
 *         D_SMTP_ERROR_TOO_LONG when the bytes do not fit, in which case the
 *         buffer is unchanged.
 */
enum d_smtp_error d_smtp_buffer_append(struct d_smtp_buffer* _buffer,
                                       const void*           _data,
                                       size_t                _length);

/**
 * @brief Appends a NUL-terminated string.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _text    the string to append.
 * @return as d_smtp_buffer_append().
 */
enum d_smtp_error d_smtp_buffer_append_text(struct d_smtp_buffer* _buffer,
                                            const char*           _text);

/**
 * @brief Appends the decimal form of a number.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _value   the number.
 * @return as d_smtp_buffer_append().
 */
enum d_smtp_error d_smtp_buffer_append_number(struct d_smtp_buffer* _buffer,
                                              size_t                _value);


//==============================================================================
// 3.  REPLIES
//==============================================================================
// A reply is a three-digit code and one or more lines of text (RFC 5321
// section 4.2); with ENHANCEDSTATUSCODES each line also opens with a
// class.subject.detail status (RFC 2034, RFC 3463).


// 3.1    Reply types
//------------------------------------------------------------------------------
// 3.1.1
// d_smtp_status
//   struct: an RFC 3463 enhanced status code. A class of 0 means the reply
// carried none.
struct d_smtp_status
{
    unsigned int status_class;  // 2, 4 or 5; 0 when absent
    unsigned int subject;       // 0 to 999
    unsigned int detail;        // 0 to 999
};

// 3.1.2
// d_smtp_reply
//   struct: one reply, assembled a line at a time. `text` holds each line's
// text -- what follows the code and its separator -- joined by '\n' and
// NUL-terminated.
struct d_smtp_reply
{
    unsigned int         code;                        // 0 before any line
    struct d_smtp_status status;                      // from the first line
    size_t               line_count;                  // lines parsed
    size_t               length;                      // bytes held in text
    bool                 complete;                    // final line parsed
    bool                 truncated;                   // text overflowed
    char                 text[D_SMTP_REPLY_TEXT_MAX];
};

// 3.2    Reply parsing
//------------------------------------------------------------------------------
// d_smtp_reply_clear -- empties a reply for reuse; a NULL reply is ignored
void              d_smtp_reply_clear(struct d_smtp_reply* _reply);

/**
 * @brief Parses one reply line into a reply under construction.
 *
 * @param[in,out] _reply   the reply being assembled; must not be complete.
 * @param[in]     _line    the line, without its CRLF.
 * @param[in]     _length  length of `_line`.
 * @post On success, `_reply->complete` records whether this was the final
 *       line. Text beyond D_SMTP_REPLY_TEXT_MAX is dropped and flagged.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument or a complete
 *         reply; D_SMTP_ERROR_PROTOCOL when the line is not a reply line, or
 *         carries a different code from the reply's first line.
 */
enum d_smtp_error d_smtp_reply_feed(struct d_smtp_reply* _reply,
                                    const char*          _line,
                                    size_t               _length);

// d_smtp_reply_line -- line `_index` of the stored text (not NUL-terminated)
// and its length; NULL past the last line kept. O(text), never fails.
const char*       d_smtp_reply_line(const struct d_smtp_reply* _reply,
                                    size_t                     _index,
                                    size_t*                    _length);

// 3.3    Reply codes
//------------------------------------------------------------------------------
// classification by first digit -- all O(1), none fail
bool              d_smtp_code_is_positive(unsigned int _code);
bool              d_smtp_code_is_intermediate(unsigned int _code);
bool              d_smtp_code_is_transient(unsigned int _code);
bool              d_smtp_code_is_permanent(unsigned int _code);

// 3.4    Reply formatting
//------------------------------------------------------------------------------
/**
 * @brief Appends a complete reply, CRLFs included, ready for a server to send.
 *
 * @param[in,out] _buffer  receives the reply.
 * @param[in]     _code    the reply code, 200 to 599.
 * @param[in]     _status  enhanced status for every line, or NULL for none.
 * @param[in]     _text    the text; each '\n' starts another reply line, and
 *                         every line but the last is sent with a hyphen.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument, a code out of
 *         range, or text containing CR; D_SMTP_ERROR_TOO_LONG when the buffer
 *         is too small, in which case it is unchanged.
 */
enum d_smtp_error d_smtp_reply_format(struct d_smtp_buffer*       _buffer,
                                      unsigned int                _code,
                                      const struct d_smtp_status* _status,
                                      const char*                 _text);


//==============================================================================
// 4.  CAPABILITIES
//==============================================================================
// What a server offers, as discovered from its EHLO reply.


// 4.1    Capability types
//------------------------------------------------------------------------------
// 4.1.1
// d_smtp_extension
//   enum: service extensions the kernel recognizes, as combinable bits.
enum d_smtp_extension
{
    D_SMTP_EXTENSION_NONE                = 0x0000u,
    D_SMTP_EXTENSION_PIPELINING          = 0x0001u,  // RFC 2920
    D_SMTP_EXTENSION_SIZE                = 0x0002u,  // RFC 1870
    D_SMTP_EXTENSION_8BITMIME            = 0x0004u,  // RFC 6152
    D_SMTP_EXTENSION_STARTTLS            = 0x0008u,  // RFC 3207
    D_SMTP_EXTENSION_AUTH                = 0x0010u,  // RFC 4954
    D_SMTP_EXTENSION_ENHANCEDSTATUSCODES = 0x0020u,  // RFC 2034
    D_SMTP_EXTENSION_SMTPUTF8            = 0x0040u,  // RFC 6531
    D_SMTP_EXTENSION_CHUNKING            = 0x0080u,  // RFC 3030
    D_SMTP_EXTENSION_DSN                 = 0x0100u   // RFC 3461
};

// 4.1.2
// d_smtp_auth
//   enum: SASL mechanisms, as combinable bits. D_SMTP_AUTH_NONE doubles as
// "choose the best one offered" where a mechanism is requested.
enum d_smtp_auth
{
    D_SMTP_AUTH_NONE     = 0x00u,
    D_SMTP_AUTH_PLAIN    = 0x01u,  // RFC 4616
    D_SMTP_AUTH_LOGIN    = 0x02u,  // draft-murchison-sasl-login
    D_SMTP_AUTH_CRAM_MD5 = 0x04u,  // RFC 2195; see D_ENV_SMTP_HAS_CRAM_MD5
    D_SMTP_AUTH_XOAUTH2  = 0x08u   // OAuth 2.0 bearer token
};

// 4.1.3
// d_smtp_capabilities
//   struct: the extensions and mechanisms a server offered in its EHLO reply.
struct d_smtp_capabilities
{
    unsigned int extensions;  // D_SMTP_EXTENSION_* bits
    unsigned int auth;        // D_SMTP_AUTH_* bits offered
    size_t       size_limit;  // declared SIZE; 0 when absent or unlimited
    bool         extended;    // EHLO succeeded (false after a HELO fallback)
};

// 4.2    Capability discovery
//------------------------------------------------------------------------------
// d_smtp_capabilities_clear -- forgets every capability; NULL is ignored
void              d_smtp_capabilities_clear(
                      struct d_smtp_capabilities* _capabilities);

/**
 * @brief Reads the extensions and SASL mechanisms from an EHLO reply.
 *
 * @param[out] _capabilities  receives the capabilities; cleared first.
 * @param[in]  _reply         a complete 250 reply to EHLO.
 * @return D_SMTP_OK, or D_SMTP_ERROR_INVALID for a NULL argument or a reply
 *         that is not a complete 250.
 */
enum d_smtp_error d_smtp_capabilities_parse(
                      struct d_smtp_capabilities* _capabilities,
                      const struct d_smtp_reply*  _reply);

// name lookups for SASL mechanisms -- O(1), none fail; an unknown mechanism
// has no name (NULL) and an unknown name maps to D_SMTP_AUTH_NONE
const char*       d_smtp_auth_name(enum d_smtp_auth _mechanism);
enum d_smtp_auth  d_smtp_auth_from_name(const char* _name,
                                        size_t      _length);


//==============================================================================
// 5.  ADDRESSES AND COMMANDS
//==============================================================================
// Validation keeps malformed or injected text off the wire; parsing is what a
// server needs to read commands. Validation also rejects CR, LF and NUL
// everywhere, which is the kernel's defence against command injection.


// 5.1    Validation
//------------------------------------------------------------------------------
// predicates -- none fail, a NULL argument is simply invalid:
//   address  a Mailbox (RFC 5321 section 4.1.2): dot-string or quoted
//            local-part, "@", and a domain or address literal
//   domain   a domain name or address literal, as EHLO takes
//   text     free text holds no CR, LF or NUL and may join a command line
// `_allow_utf8` admits UTF-8 in local-parts and domain labels (RFC 6531).
bool              d_smtp_address_is_valid(const char* _address,
                                          size_t      _length,
                                          bool        _allow_utf8);
bool              d_smtp_domain_is_valid(const char* _domain,
                                         size_t      _length,
                                         bool        _allow_utf8);
bool              d_smtp_text_is_safe(const char* _text,
                                      size_t      _length);

// 5.2    Command types
//------------------------------------------------------------------------------
// 5.2.1
// d_smtp_verb
//   enum: the commands a server recognizes.
enum d_smtp_verb
{
    D_SMTP_VERB_UNKNOWN = 0,
    D_SMTP_VERB_HELO,
    D_SMTP_VERB_EHLO,
    D_SMTP_VERB_MAIL,
    D_SMTP_VERB_RCPT,
    D_SMTP_VERB_DATA,
    D_SMTP_VERB_RSET,
    D_SMTP_VERB_NOOP,
    D_SMTP_VERB_QUIT,
    D_SMTP_VERB_VRFY,
    D_SMTP_VERB_HELP,
    D_SMTP_VERB_STARTTLS,
    D_SMTP_VERB_AUTH
};

// 5.2.2
// d_smtp_command
//   struct: a parsed command line. `argument` points into the parsed line and
// is not NUL-terminated.
struct d_smtp_command
{
    enum d_smtp_verb verb;
    const char*      argument;
    size_t           argument_length;
};

// 5.2.3
// d_smtp_path
//   struct: the path argument of MAIL or RCPT. Both members point into the
// parsed argument; `address` may be empty (the null reverse-path), and
// `parameters` is NULL when no ESMTP parameters follow.
struct d_smtp_path
{
    const char* address;
    size_t      address_length;
    const char* parameters;
    size_t      parameters_length;
};

// 5.2.4
// d_smtp_parameter
//   struct: one ESMTP parameter, "keyword[=value]"; `value` is NULL when the
// parameter carries none.
struct d_smtp_parameter
{
    const char* keyword;
    size_t      keyword_length;
    const char* value;
    size_t      value_length;
};

// 5.3    Command parsing
//------------------------------------------------------------------------------
/**
 * @brief Splits a command line into its verb and argument.
 *
 * @param[in]  _line     the line, without its CRLF.
 * @param[in]  _length   length of `_line`.
 * @param[out] _command  receives the verb (D_SMTP_VERB_UNKNOWN when it is not
 *                       recognized) and the trimmed argument.
 * @return D_SMTP_OK, or D_SMTP_ERROR_INVALID for a NULL argument.
 */
enum d_smtp_error d_smtp_command_parse(const char*            _line,
                                       size_t                 _length,
                                       struct d_smtp_command* _command);

/**
 * @brief Parses "FROM:<path> parameters" or "TO:<path> parameters".
 *
 * @note A source route ("@a,@b:user@c") is accepted and ignored, as RFC 5321
 *       section 4.1.1.3 requires; a space after the colon is tolerated.
 *
 * @param[in]  _argument  the command's argument.
 * @param[in]  _length    length of `_argument`.
 * @param[in]  _keyword   "FROM:" or "TO:", matched without regard to case.
 * @param[out] _path      receives the address and parameters.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument; or
 *         D_SMTP_ERROR_PROTOCOL when the argument is malformed.
 */
enum d_smtp_error d_smtp_path_parse(const char*         _argument,
                                    size_t              _length,
                                    const char*         _keyword,
                                    struct d_smtp_path* _path);

// d_smtp_keyword_is -- whether a counted span equals a keyword under ASCII
// case folding; never fails, and a NULL argument is simply unequal
bool              d_smtp_keyword_is(const char* _text,
                                    size_t      _length,
                                    const char* _keyword);

// d_smtp_parameter_next -- reads the parameter at *_cursor and advances past
// it; false once [*_cursor, _end) holds no further parameter. Never fails.
bool              d_smtp_parameter_next(const char**             _cursor,
                                        const char*              _end,
                                        struct d_smtp_parameter* _parameter);


//==============================================================================
// 6.  ENCODINGS
//==============================================================================
// Base64 carries SASL exchanges; the data encoder applies transparency (dot
// doubling) and normalizes every line ending to CRLF, since RFC 5321 forbids
// bare CR and bare LF on the wire.


// 6.1    Base64
//------------------------------------------------------------------------------
// d_smtp_base64_size -- encoded length of `_length` bytes, padding included
size_t            d_smtp_base64_size(size_t _length);

/**
 * @brief Appends the base64 encoding of a byte range.
 *
 * @param[in]     _data    the bytes; may be NULL when `_length` is 0.
 * @param[in]     _length  number of bytes.
 * @param[in,out] _buffer  receives the encoding.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument; or
 *         D_SMTP_ERROR_TOO_LONG when the encoding does not fit.
 */
enum d_smtp_error d_smtp_base64_encode(const void*           _data,
                                       size_t                _length,
                                       struct d_smtp_buffer* _buffer);

/**
 * @brief Appends the bytes a base64 text decodes to.
 *
 * @note Decoding is strict: no whitespace, and padding exactly as RFC 4648
 *       section 4 specifies.
 *
 * @param[in]     _text    the base64 text.
 * @param[in]     _length  length of `_text`.
 * @param[in,out] _buffer  receives the decoded bytes, NUL-terminated.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument;
 *         D_SMTP_ERROR_PROTOCOL for malformed text; or D_SMTP_ERROR_TOO_LONG
 *         when the result does not fit.
 */
enum d_smtp_error d_smtp_base64_decode(const char*           _text,
                                       size_t                _length,
                                       struct d_smtp_buffer* _buffer);

// 6.2    Message data
//------------------------------------------------------------------------------
// 6.2.1
// d_smtp_data_encoder
//   struct: streaming state of the DATA encoder, carried across calls so a
// message can be encoded in chunks of any size.
struct d_smtp_data_encoder
{
    bool line_start;  // the next byte begins a line
    bool after_cr;    // the last byte was a CR already sent as CRLF
};

// d_smtp_data_encoder_init -- resets an encoder to the start of a message
void              d_smtp_data_encoder_init(
                      struct d_smtp_data_encoder* _encoder);

// d_smtp_data_encode -- encodes as much of the input as fits in the buffer
// (every input byte needs at most two output bytes) and returns how many
// input bytes it consumed; 0 for a NULL argument
size_t            d_smtp_data_encode(struct d_smtp_data_encoder* _encoder,
                                     const char*                 _input,
                                     size_t                      _length,
                                     struct d_smtp_buffer*       _buffer);

/**
 * @brief Appends the end-of-data marker, ending an unterminated last line.
 *
 * @param[in,out] _encoder  the encoder; reset on success.
 * @param[in,out] _buffer   receives at most five bytes.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument; or
 *         D_SMTP_ERROR_TOO_LONG when the marker does not fit.
 */
enum d_smtp_error d_smtp_data_finish(struct d_smtp_data_encoder* _encoder,
                                     struct d_smtp_buffer*       _buffer);

// d_smtp_data_unstuff -- for a server reading DATA: true when the line is the
// end-of-data marker; otherwise the line's payload, a leading transparency
// dot removed. Every pointer is required.
bool              d_smtp_data_unstuff(const char*  _line,
                                      size_t       _length,
                                      const char** _payload,
                                      size_t*      _payload_length);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_COMMON_H
