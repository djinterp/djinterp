/*******************************************************************************
* djinterp [net]                                                         http1.h
*
* HTTP/1.1's message syntax, as RFC 9112 defines it: message heads, in C.
*   A head is found line by line as bytes arrive, each call resuming where
* the last stopped, and parsed whole once the empty line ending it arrives,
* so the work done is proportional to the bytes fed, however they are split.
* Its parts are views into the caller's bytes. Nothing reads a socket,
* allocates, or blocks, so blocking and non-blocking drivers share it, and
* it can be fuzzed alone. It supplies
*     - the parser, fields, and parsed request and response heads          [1]
*     - parsers set up and reset                                           [2]
*     - request and response heads parsed                                  [3]
*     - fields found by name                                               [4]
*   Parsing is strict throughout: a bare CR or LF, a folded line, whitespace
* before a field's colon, a malformed or misplaced request-target, and a
* missing or repeated Host in HTTP/1.1 are all refused, since request
* smuggling lives in the gaps between parsers that would accept them.
*
*
* path:      /inc/djinterp/net/http/http1.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The parser
         1.  d_http1_parser
    2.  Heads
         1.  d_http1_field
         2.  d_http1_fields
         3.  d_http1_target_form
         4.  d_http1_request
         5.  d_http1_response
2.  PARSERS
    -------
    1.  Setup
3.  PARSING
    -------
    1.  Requests
    2.  Responses
4.  FIELDS
    ------
    1.  Lookup
*/

#ifndef DJINTERP_NET_HTTP_HTTP1_H
#define DJINTERP_NET_HTTP_HTTP1_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_text
#include "./http.h"                    // d_http_error, methods, versions


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The parser
//------------------------------------------------------------------------------
// 1.1.1
// d_http1_parser
//   struct: the limits a parse enforces, and how far the head being sought
// has been scanned. Set the limits after d_http1_parser_init; the scan state
// belongs to the parser.
struct d_http1_parser
{
    size_t start_line_max;  // longest start line, CRLF excluded
    size_t head_max;        // longest head, empty lines before it included
    size_t line;            // where the next line to scan begins
    size_t start;           // where the start line begins, once found
    bool   started;         // whether the start line has been found
};

// 1.2    Heads
//------------------------------------------------------------------------------
// 1.2.1
// d_http1_field
//   struct: one field line: its name, and its value without the whitespace
// around it.
struct d_http1_field
{
    struct d_pack_text name;
    struct d_pack_text value;
};

// 1.2.2
// d_http1_fields
//   struct: the caller's array of field lines. The caller sets items and
// capacity; a parse sets count, keeping the lines' order.
struct d_http1_fields
{
    struct d_http1_field* items;
    size_t                capacity;
    size_t                count;
};

// 1.2.3
// d_http1_target_form
//   enum: which of a request-target's four forms a request used (RFC 9112,
// section 3.2).
enum d_http1_target_form
{
    D_HTTP1_TARGET_ORIGIN = 0,  // an absolute path and query: "/a?b"
    D_HTTP1_TARGET_ABSOLUTE,    // an absolute URI, as sent to a proxy
    D_HTTP1_TARGET_AUTHORITY,   // host and port, CONNECT's alone
    D_HTTP1_TARGET_ASTERISK     // "*", OPTIONS' alone
};

// 1.2.4
// d_http1_request
//   struct: a parsed request head; every text a view into the parsed bytes.
struct d_http1_request
{
    enum d_http_method       method;       // OTHER for an extension method
    struct d_pack_text       method_name;  // as written
    struct d_pack_text       target;       // the request-target as written
    enum d_http1_target_form target_form;
    enum d_http_version      version;
    struct d_http1_fields    fields;       // items and capacity the caller's
};

// 1.2.5
// d_http1_response
//   struct: a parsed response head; every text a view into the parsed
// bytes.
struct d_http1_response
{
    enum d_http_version   version;
    unsigned int          status;
    struct d_pack_text    reason;  // perhaps empty; it carries no meaning
    struct d_http1_fields fields;  // items and capacity the caller's
};


//==============================================================================
// 2.  PARSERS
//==============================================================================


// 2.1    Setup
//------------------------------------------------------------------------------
// init -- cfg_http.h's limits, and nothing scanned; reset -- nothing
// scanned, limits kept, to parse a head that begins elsewhere, or the same
// bytes after they have moved
void                 d_http1_parser_init(struct d_http1_parser* _parser);
void                 d_http1_parser_reset(struct d_http1_parser* _parser);


//==============================================================================
// 3.  PARSING
//==============================================================================
// Both parsers take the bytes received so far, from the head's start. While
// they answer D_HTTP_ERROR_INCOMPLETE, call again with the same bytes and
// more after them; the parser resumes where it stopped. Any other answer
// resets it, ready for the head after this one.


// 3.1    Requests
//------------------------------------------------------------------------------
/**
 * @brief Parses a request head (RFC 9112, sections 2, 3, and 5); empty lines
 *        before the request line are skipped (RFC 9112, 2.2).
 *
 * @param[in,out] _parser    the limits, and the scan's state.
 * @param[in]     _data      the bytes received so far.
 * @param[in,out] _out       the caller sets fields.items and
 *                           fields.capacity; receives the head.
 * @param[out]    _consumed  receives the head's length, the empty line
 *                           ending it included; bytes after it are the body
 *                           or the next message.
 * @return D_HTTP_OK; D_HTTP_ERROR_INCOMPLETE for more bytes;
 *         D_HTTP_ERROR_ARGUMENT; or the error of the first fault found:
 *         a start line malformed, too long, or of a bad method, target, or
 *         version; a head too large; a bare CR or LF; a folded line; a
 *         field name or value refused; too many fields; or, in HTTP/1.1, not
 *         exactly one Host holding an authority without userinfo.
 */
enum d_http_error    d_http1_parse_request(struct d_http1_parser*  _parser,
                                           struct d_pack_text      _data,
                                           struct d_http1_request* _out,
                                           size_t*                 _consumed);

// 3.2    Responses
//------------------------------------------------------------------------------
/**
 * @brief Parses a response head (RFC 9112, sections 2, 4, and 5); an empty
 *        reason may lack the space before it, since it carries no meaning.
 *
 * @param[in,out] _parser    the limits, and the scan's state.
 * @param[in]     _data      the bytes received so far.
 * @param[in,out] _out       the caller sets fields.items and
 *                           fields.capacity; receives the head.
 * @param[out]    _consumed  receives the head's length, the empty line
 *                           ending it included.
 * @return D_HTTP_OK; D_HTTP_ERROR_INCOMPLETE for more bytes;
 *         D_HTTP_ERROR_ARGUMENT; or the error of the first fault found, as
 *         for requests, with a status code refused in place of a method or
 *         target, and no Host rule.
 */
enum d_http_error    d_http1_parse_response(struct d_http1_parser*   _parser,
                                            struct d_pack_text       _data,
                                            struct d_http1_response* _out,
                                            size_t*                  _consumed);


//==============================================================================
// 4.  FIELDS
//==============================================================================


// 4.1    Lookup
//------------------------------------------------------------------------------
// d_http1_fields_index -- the index of the first field at or after _from
// whose name matches, ASCII case ignored, or count if none does; call again
// from the index after a match to find the next
size_t               d_http1_fields_index(const struct d_http1_fields* _fields,
                                          struct d_pack_text           _name,
                                          size_t                       _from);


D_EXTERN_C_END


#endif  // DJINTERP_NET_HTTP_HTTP1_H
