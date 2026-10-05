/*******************************************************************************
* djinterp [net]                                                     ftp_reply.h
*
* FTP replies (RFC 959 4.2): codes, parsing, and formatting.
*   Classifies reply codes; parses control-connection bytes, in chunks of any
* size, into complete replies, multi-line replies and Telnet sequences
* included; formats replies so that no text can forge a closing line; and maps
* reply codes onto errors.
*
*
* path:      /inc/djinterp/net/ftp/ftp_reply.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Reply codes
         1.  d_ftp_reply_class
         2.  d_ftp_reply_category
         3.  d_ftp_reply_code
    2.  Replies and parsing
         1.  d_ftp_reply
         2.  d_ftp_status
         3.  d_ftp_reply_parser
2.  REPLIES
    -------
    1.  Classification
    2.  Parsing
    3.  Formatting
    4.  Errors
*/

#ifndef DJINTERP_NET_FTP_FTP_REPLY_H
#define DJINTERP_NET_FTP_FTP_REPLY_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_span, d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_reply.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Reply codes
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_reply_class
//   enum: the first digit of a reply code (RFC 959 4.2.1), which alone decides
// how a client proceeds; PROTECTED is RFC 2228's class for wrapped replies.
enum d_ftp_reply_class
{
    D_FTP_REPLY_CLASS_INVALID      = 0,  // not a reply code
    D_FTP_REPLY_CLASS_PRELIMINARY  = 1,  // 1yz: another reply will follow
    D_FTP_REPLY_CLASS_COMPLETION   = 2,  // 2yz: the action succeeded
    D_FTP_REPLY_CLASS_INTERMEDIATE = 3,  // 3yz: send the next command
    D_FTP_REPLY_CLASS_TRANSIENT    = 4,  // 4yz: failed; may succeed later
    D_FTP_REPLY_CLASS_PERMANENT    = 5,  // 5yz: failed; do not just retry
    D_FTP_REPLY_CLASS_PROTECTED    = 6   // 6yz: an RFC 2228 wrapped reply
};

// 1.1.2
// d_ftp_reply_category
//   enum: the second digit of a reply code, naming what the reply concerns.
enum d_ftp_reply_category
{
    D_FTP_REPLY_CATEGORY_SYNTAX         = 0,  // x0z
    D_FTP_REPLY_CATEGORY_INFORMATION    = 1,  // x1z
    D_FTP_REPLY_CATEGORY_CONNECTIONS    = 2,  // x2z
    D_FTP_REPLY_CATEGORY_AUTHENTICATION = 3,  // x3z
    D_FTP_REPLY_CATEGORY_UNSPECIFIED    = 4,  // x4z
    D_FTP_REPLY_CATEGORY_FILE_SYSTEM    = 5,  // x5z
    D_FTP_REPLY_CATEGORY_INVALID        = 15  // not a reply code
};

// 1.1.3
// d_ftp_reply_code
//   enum: the reply codes of RFC 959 and of the extensions this module
// covers (RFC 2228, 2428, 1639), named so code can say what it means.
enum d_ftp_reply_code
{
    D_FTP_REPLY_RESTART_MARKER           = 110,
    D_FTP_REPLY_SERVICE_READY_IN         = 120,
    D_FTP_REPLY_TRANSFER_STARTING        = 125,
    D_FTP_REPLY_OPENING_DATA             = 150,
    D_FTP_REPLY_COMMAND_OK               = 200,
    D_FTP_REPLY_SUPERFLUOUS              = 202,
    D_FTP_REPLY_SYSTEM_STATUS            = 211,
    D_FTP_REPLY_DIRECTORY_STATUS         = 212,
    D_FTP_REPLY_FILE_STATUS              = 213,
    D_FTP_REPLY_HELP                     = 214,
    D_FTP_REPLY_SYSTEM_TYPE              = 215,
    D_FTP_REPLY_SERVICE_READY            = 220,
    D_FTP_REPLY_CLOSING_CONTROL          = 221,
    D_FTP_REPLY_DATA_OPEN                = 225,
    D_FTP_REPLY_CLOSING_DATA             = 226,
    D_FTP_REPLY_PASSIVE                  = 227,
    D_FTP_REPLY_LONG_PASSIVE             = 228,
    D_FTP_REPLY_EXTENDED_PASSIVE         = 229,
    D_FTP_REPLY_LOGGED_IN                = 230,
    D_FTP_REPLY_SECURITY_LOGGED_IN       = 232,
    D_FTP_REPLY_SECURITY_ACCEPTED        = 234,
    D_FTP_REPLY_SECURITY_DATA_DONE       = 235,
    D_FTP_REPLY_FILE_ACTION_OK           = 250,
    D_FTP_REPLY_PATHNAME                 = 257,
    D_FTP_REPLY_NEED_PASSWORD            = 331,
    D_FTP_REPLY_NEED_ACCOUNT             = 332,
    D_FTP_REPLY_SECURITY_MECHANISM_OK    = 334,
    D_FTP_REPLY_SECURITY_DATA_MORE       = 335,
    D_FTP_REPLY_NEED_PASSWORD_CHALLENGE  = 336,
    D_FTP_REPLY_FILE_ACTION_PENDING      = 350,
    D_FTP_REPLY_SERVICE_UNAVAILABLE      = 421,
    D_FTP_REPLY_CANNOT_OPEN_DATA         = 425,
    D_FTP_REPLY_TRANSFER_ABORTED         = 426,
    D_FTP_REPLY_SECURITY_RESOURCE        = 431,
    D_FTP_REPLY_FILE_BUSY                = 450,
    D_FTP_REPLY_LOCAL_ERROR              = 451,
    D_FTP_REPLY_INSUFFICIENT_STORAGE     = 452,
    D_FTP_REPLY_SYNTAX_ERROR             = 500,
    D_FTP_REPLY_ARGUMENT_ERROR           = 501,
    D_FTP_REPLY_NOT_IMPLEMENTED          = 502,
    D_FTP_REPLY_BAD_SEQUENCE             = 503,
    D_FTP_REPLY_PARAMETER_UNSUPPORTED    = 504,
    D_FTP_REPLY_PROTOCOL_UNSUPPORTED     = 522,
    D_FTP_REPLY_NOT_LOGGED_IN            = 530,
    D_FTP_REPLY_NEED_ACCOUNT_TO_STORE    = 532,
    D_FTP_REPLY_PROTECTION_DENIED        = 533,
    D_FTP_REPLY_POLICY_DENIED            = 534,
    D_FTP_REPLY_SECURITY_CHECK_FAILED    = 535,
    D_FTP_REPLY_PROT_UNSUPPORTED         = 536,
    D_FTP_REPLY_COMMAND_PROT_UNSUPPORTED = 537,
    D_FTP_REPLY_FILE_UNAVAILABLE         = 550,
    D_FTP_REPLY_PAGE_TYPE_UNKNOWN        = 551,
    D_FTP_REPLY_STORAGE_EXCEEDED         = 552,
    D_FTP_REPLY_NAME_NOT_ALLOWED         = 553,
    D_FTP_REPLY_INTEGRITY_PROTECTED      = 631,
    D_FTP_REPLY_PRIVATE_PROTECTED        = 632,
    D_FTP_REPLY_CONFIDENTIAL_PROTECTED   = 633
};

// 1.2    Replies and parsing
//------------------------------------------------------------------------------
// 1.2.1
// d_ftp_reply
//   struct: one complete reply. The code prefixes are removed from every line
// that carried one, and the lines are joined by single '\n' bytes, so the
// text of "211-Features:\r\n EPSV\r\n211 End\r\n" is "Features:\n EPSV\nEnd".
struct d_ftp_reply
{
    unsigned          code;        // the three-digit reply code
    size_t            line_count;  // 1 for a single-line reply
    struct d_ftp_span text;        // the lines, codes removed, '\n'-joined
    bool              truncated;   // text was cut short to fit the storage
};

// 1.2.2
// d_ftp_status
//   enum: the state of an incremental parse after it consumes input.
enum d_ftp_status
{
    D_FTP_STATUS_PENDING = 0,  // all input consumed; the item is incomplete
    D_FTP_STATUS_COMPLETE,     // an item completed; input may remain
    D_FTP_STATUS_FAILED        // the input is not well-formed
};

// 1.2.3
// d_ftp_reply_parser
//   struct: an incremental reply parser. It accepts control-connection bytes
// in chunks of any size, filters Telnet command sequences, and completes one
// reply at a time into caller-provided text storage. Its fields are private;
// use the functions in section 2.2.
struct d_ftp_reply_parser
{
    char*         storage;       // caller's text storage
    size_t        capacity;      // size of `storage` in bytes
    size_t        length;        // text held for the current reply
    size_t        line_length;   // bytes seen on the current line
    size_t        line_count;    // lines finished in the current reply
    unsigned      code;          // the current reply's code; 0 before it
    char          code_text[3];  // the code's digits as received
    char          prefix[4];     // the current line's first four bytes
    unsigned char telnet;        // Telnet filter state
    bool          truncated;     // the text outgrew `storage`
    bool          failed;        // a malformed line was seen
};


//==============================================================================
// 2.  REPLIES
//==============================================================================


// 2.1    Classification
//------------------------------------------------------------------------------
// Pure functions of a reply code. A code is valid when its first digit is 1
// through 6 and its second 0 through 5; the parser is more lenient and
// accepts any second digit, as servers occasionally send one.
bool                      d_ftp_reply_is_valid(unsigned _code);
enum d_ftp_reply_class    d_ftp_reply_class_of(unsigned _code);
enum d_ftp_reply_category d_ftp_reply_category_of(unsigned _code);
/**
 * @brief Returns RFC 959's standard text for a reply code.
 *
 * @param[in] _code the reply code.
 * @return a static string, or NULL for a code with no standard text.
 */
const char*               d_ftp_reply_text(unsigned _code);

// 2.2    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Prepares a reply parser over caller-provided text storage.
 *
 * Text beyond `_capacity - 1` bytes is dropped and the reply is marked
 * truncated; the code and completion are unaffected, so zero capacity (and a
 * NULL `_storage`) gives a parser that reads codes alone.
 *
 * @param[out] _parser   the parser to prepare.
 * @param[in]  _storage  text storage; must outlive the parser's use.
 * @param[in]  _capacity the size of `_storage` in bytes.
 */
void              d_ftp_reply_parser_init(struct d_ftp_reply_parser* _parser,
                                          char*                      _storage,
                                          size_t                     _capacity);
/**
 * @brief Discards any partial reply and clears a failure.
 *
 * @param[in,out] _parser the parser.
 */
void              d_ftp_reply_parser_reset(struct d_ftp_reply_parser* _parser);
/**
 * @brief Consumes control-connection bytes until a reply completes.
 *
 * Accepts CR LF or a bare LF as the line end, drops stray CR and NUL bytes,
 * and filters Telnet command sequences, so chunks may split anywhere, even
 * inside one. Parsing stops at the end of a reply so pipelined replies are
 * returned one per call; feed the unconsumed rest again.
 *
 * @pre     `_parser` was prepared with d_ftp_reply_parser_init().
 * @post    on D_FTP_STATUS_COMPLETE, `_out->text` points into the
 *          parser's storage and stays valid until the next feed or reset.
 *          After D_FTP_STATUS_FAILED, every feed fails until a reset.
 *
 * @param[in,out] _parser       the parser.
 * @param[in]     _data         received bytes; NULL only if `_length` is 0.
 * @param[in]     _length       their count.
 * @param[out]    _out_used     bytes consumed before returning.
 * @param[out]    _out          the reply, on D_FTP_STATUS_COMPLETE.
 * @return D_FTP_STATUS_COMPLETE when a reply completed,
 *         D_FTP_STATUS_PENDING when more input is needed, or
 *         D_FTP_STATUS_FAILED for a line that cannot begin a reply or for a
 *         NULL argument.
 */
enum d_ftp_status d_ftp_reply_parser_feed(struct d_ftp_reply_parser* _parser,
                                          const char*                _data,
                                          size_t                     _length,
                                          size_t*                    _out_used,
                                          struct d_ftp_reply*        _out);

// 2.3    Formatting
//------------------------------------------------------------------------------
/**
 * @brief Appends a reply, single- or multi-line, ready to send.
 *
 * Each line of `_text` (split at CR, LF, or CR LF) becomes one reply line:
 * the first carries "ddd-" when more follow, the last "ddd ", and any line
 * between that begins with a digit gains a leading space so a client cannot
 * mistake it for the end (RFC 959 4.2). Line breaks in `_text` therefore
 * never inject a reply of their own.
 *
 * @param[in]     _code the reply code; must satisfy d_ftp_reply_is_valid().
 * @param[in]     _text the text; NULL uses d_ftp_reply_text(), or "".
 * @param[in,out] _out  the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error  d_ftp_reply_format(unsigned             _code,
                                     const char*          _text,
                                     struct d_ftp_buffer* _out);

// 2.4    Errors
//------------------------------------------------------------------------------
/**
 * @brief Maps a reply code onto the error a failed exchange should report.
 *
 * Positive replies (1yz, 2yz, 3yz) map to D_FTP_OK. RFC 2228 protected
 * replies map to D_FTP_ERROR_UNSUPPORTED: they must be unwrapped by the
 * security mechanism before they mean anything.
 *
 * @param[in] _code the reply code.
 * @return the matching error; D_FTP_ERROR_MALFORMED for a non-code.
 */
enum d_ftp_error d_ftp_error_from_reply(unsigned _code);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_REPLY_H
