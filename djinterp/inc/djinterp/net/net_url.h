/*******************************************************************************
* djinterp [net]                                                       net_url.h
*
* URI references as RFC 3986 defines them.
*   Parsing into views of the caller's text, recomposition, resolution against
* a base, normalization, percent-encoding, and the step from a URL to an
* endpoint. It builds on net/net.h, whose ports, endpoints, and errors it uses,
* and supplies
*     - the error, host, and part vocabulary, and the parsed URL           [1]
*     - parsing any URI reference, and recomposing one                     [2]
*     - resolution against a base, and normalization for comparison        [3]
*     - percent-encoding and decoding, per component                       [4]
*     - registered schemes' ports, and a URL's endpoint                    [5]
*   Nothing allocates. A parsed URL points into the text it was parsed from,
* which must outlive it. A function producing text writes it, NUL-terminated,
* to the caller's buffer, and reports a length that suffices even when the
* buffer is too small, so one call can size a buffer and a second fill it.
*   Hosts are ASCII, as RFC 3986's registered names are; internationalized
* domain names (IDNA) are not converted.
*
*
* path:      /inc/djinterp/net/net_url.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
         1.  d_net_url_error
         2.  d_net_url_host
         3.  d_net_url_part
    2.  The parsed URL
         1.  d_net_url
2.  PARSING AND RECOMPOSITION
    -------------------------
    1.  Parsing
    2.  Recomposition
3.  RESOLUTION AND NORMALIZATION
    ----------------------------
    1.  Resolution
    2.  Normalization
4.  PERCENT-ENCODING
    ----------------
    1.  Encoding and decoding
5.  SCHEMES AND ENDPOINTS
    ---------------------
    1.  Schemes
    2.  Endpoints
*/

#ifndef DJINTERP_NET_NET_URL_H
#define DJINTERP_NET_NET_URL_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../c/djinterp.h"          // framework root
#include "../c/util/sink_common.h"  // d_pack_text
#include "./net.h"                  // d_net_port, d_net_endpoint, d_net_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// Why a URL was refused, what kind of host it names, which component an
// encoding is for, and the parsed URL itself.


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// d_net_url_error
//   enum: why a URL was refused, or why an output did not fit.
enum d_net_url_error
{
    D_NET_URL_OK = 0,            // no error
    D_NET_URL_ERROR_ARGUMENT,    // a NULL argument
    D_NET_URL_ERROR_SCHEME,      // a malformed scheme, or a colon in a
                                 // relative reference's first segment
    D_NET_URL_ERROR_USERINFO,    // a character userinfo may not hold
    D_NET_URL_ERROR_HOST,        // a character a registered name may not hold
    D_NET_URL_ERROR_IP_LITERAL,  // a malformed bracketed address or zone
    D_NET_URL_ERROR_PORT,        // a port not all digits, or above 65535
    D_NET_URL_ERROR_PATH,        // a character a path may not hold
    D_NET_URL_ERROR_QUERY,       // a character a query may not hold
    D_NET_URL_ERROR_FRAGMENT,    // a character a fragment may not hold
    D_NET_URL_ERROR_PERCENT,     // a '%' not followed by two hex digits
    D_NET_URL_ERROR_BASE,        // resolving against a relative base
    D_NET_URL_ERROR_BUFFER       // the output did not fit
};

// 1.1.2
// d_net_url_host
//   enum: what kind of host a URL's authority names.
enum d_net_url_host
{
    D_NET_URL_HOST_NONE = 0,  // no authority
    D_NET_URL_HOST_NAME,      // a registered name, usually DNS; may be empty
    D_NET_URL_HOST_IPV4,      // dotted-decimal, without leading zeros
    D_NET_URL_HOST_IPV6,      // bracketed, with an optional RFC 6874 zone
    D_NET_URL_HOST_IPVFUTURE  // bracketed "v<hex>.<text>"
};

// 1.1.3
// d_net_url_part
//   enum: the component a percent-encoding is for; each leaves a different
// set of characters unencoded.
enum d_net_url_part
{
    D_NET_URL_PART_USERINFO = 0,  // unreserved, sub-delims, ':'
    D_NET_URL_PART_HOST,          // unreserved, sub-delims
    D_NET_URL_PART_PATH,          // a whole path: '/' kept
    D_NET_URL_PART_SEGMENT,       // one path segment: '/' encoded
    D_NET_URL_PART_QUERY,         // a whole query: '/' and '?' kept
    D_NET_URL_PART_FRAGMENT,      // as a query
    D_NET_URL_PART_COMPONENT      // unreserved only: a key or value the
                                  // caller places inside a query or path
};

// 1.2    The parsed URL
//------------------------------------------------------------------------------
// 1.2.1
// d_net_url
//   struct: a parsed URI reference. Every text is a view into the parsed
// text, delimiters excluded. The has_* flags tell an absent component from
// an empty one ("http://h?" from "http://h"), which recomposition keeps.
struct d_net_url
{
    struct d_pack_text  text;           // the whole reference
    struct d_pack_text  scheme;         // empty in a relative reference
    struct d_pack_text  userinfo;       // before the '@'
    struct d_pack_text  host;           // an IP literal without its brackets
    struct d_pack_text  port_text;      // the digits after ':', perhaps none
    struct d_pack_text  path;           // perhaps empty
    struct d_pack_text  query;          // after the '?'
    struct d_pack_text  fragment;       // after the '#'
    d_net_port          port;           // the explicit port, or 0
    enum d_net_url_host host_kind;      // D_NET_URL_HOST_NONE without "//"
    bool                has_authority;  // "//" present
    bool                has_userinfo;   // '@' present
    bool                has_port;       // ':' after the host present
    bool                has_query;      // '?' present
    bool                has_fragment;   // '#' present
};


//==============================================================================
// 2.  PARSING AND RECOMPOSITION
//==============================================================================
// Any URI reference in, its components out, and back again. A parsed URL
// recomposes to the text it was parsed from.


// 2.1    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Parses a URI reference: an absolute URI or a relative reference.
 *
 * @param[in]  _text      the reference; the result points into it.
 * @param[out] _out       receives the components; zeroed on failure.
 * @param[out] _error_at  receives the offset of the first character no
 *                        grammar allows; may be NULL.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, or the error of the
 *         component holding the refused character.
 */
enum d_net_url_error d_net_url_parse(struct d_pack_text _text,
                                     struct d_net_url*  _out,
                                     size_t*            _error_at);

/**
 * @brief Parses an authority standing alone, "[userinfo@]host[:port]", as
 *        HTTP's Host field and CONNECT's request-target carry one.
 *
 * @param[in]  _text      the authority; the result points into it.
 * @param[out] _out       receives the userinfo, host, and port, with
 *                        has_authority set; zeroed on failure.
 * @param[out] _error_at  receives the offset of the first character
 *                        refused; may be NULL.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, or the error of the part
 *         holding the refused character.
 */
enum d_net_url_error d_net_url_parse_authority(struct d_pack_text _text,
                                               struct d_net_url*  _out,
                                               size_t*            _error_at);

// d_net_url_is_absolute -- whether a parsed reference has a scheme
bool                 d_net_url_is_absolute(const struct d_net_url* _url);

// 2.2    Recomposition
//------------------------------------------------------------------------------
/**
 * @brief Recomposes a parsed reference (RFC 3986, section 5.3).
 *
 * @param[in]  _url       the reference.
 * @param[out] _buffer    receives the text, NUL-terminated.
 * @param[in]  _capacity  its size, the terminator included.
 * @param[out] _length    receives the text's length, the terminator
 *                        excluded, whether or not it fit.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, or
 *         D_NET_URL_ERROR_BUFFER.
 */
enum d_net_url_error d_net_url_format(const struct d_net_url* _url,
                                      char*                   _buffer,
                                      size_t                  _capacity,
                                      size_t*                 _length);


//==============================================================================
// 3.  RESOLUTION AND NORMALIZATION
//==============================================================================
// Where a link or a redirect leads, and the one text a URL has for
// comparison.


// 3.1    Resolution
//------------------------------------------------------------------------------
/**
 * @brief Resolves a reference against an absolute base (RFC 3986, section
 *        5.2, strictly): the target of a link or a redirect.
 *
 * @param[in]  _base       an absolute reference.
 * @param[in]  _reference  the reference to resolve.
 * @param[out] _buffer     receives the target's text, NUL-terminated.
 * @param[in]  _capacity   its size, the terminator included.
 * @param[out] _length     receives the target's length; on
 *                         D_NET_URL_ERROR_BUFFER, a length that suffices.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, D_NET_URL_ERROR_BASE for
 *         a relative base, or D_NET_URL_ERROR_BUFFER.
 */
enum d_net_url_error d_net_url_resolve(const struct d_net_url* _base,
                                       const struct d_net_url* _reference,
                                       char*                   _buffer,
                                       size_t                  _capacity,
                                       size_t*                 _length);

// 3.2    Normalization
//------------------------------------------------------------------------------
/**
 * @brief Normalizes a reference for comparison (RFC 3986, section 6.2).
 *
 * The scheme and a registered or IP host are lowercased; percent escapes
 * of unreserved characters are decoded and the rest uppercased; an
 * absolute reference loses its dot segments; a port that is empty or the
 * scheme's default is dropped, and leading zeros with it; an http, https,
 * ws, or wss URL with an authority and no path gets "/".
 *
 * @param[in]  _url       the reference.
 * @param[out] _buffer    receives the normalized text, NUL-terminated.
 * @param[in]  _capacity  its size, the terminator included.
 * @param[out] _length    receives the text's length; on
 *                        D_NET_URL_ERROR_BUFFER, a length that suffices.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, or
 *         D_NET_URL_ERROR_BUFFER.
 */
enum d_net_url_error d_net_url_normalize(const struct d_net_url* _url,
                                         char*                   _buffer,
                                         size_t                  _capacity,
                                         size_t*                 _length);


//==============================================================================
// 4.  PERCENT-ENCODING
//==============================================================================
// Raw bytes to the text a component may hold, and back.


// 4.1    Encoding and decoding
//------------------------------------------------------------------------------
/**
 * @brief Percent-encodes raw bytes for one component, with uppercase hex;
 *        a '%' in the input is encoded too.
 *
 * @param[in]  _text      the raw bytes.
 * @param[in]  _part      the component they are for.
 * @param[out] _buffer    receives the encoding, NUL-terminated.
 * @param[in]  _capacity  its size, the terminator included.
 * @param[out] _length    receives the encoding's length, whether or not it
 *                        fit.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT, or
 *         D_NET_URL_ERROR_BUFFER.
 */
enum d_net_url_error d_net_url_encode(struct d_pack_text  _text,
                                      enum d_net_url_part _part,
                                      char*               _buffer,
                                      size_t              _capacity,
                                      size_t*             _length);

/**
 * @brief Decodes percent escapes; everything else is copied as it is.
 *
 * @param[in]  _text      the encoded text.
 * @param[out] _buffer    receives the bytes, NUL-terminated; the bytes may
 *                        themselves hold a NUL.
 * @param[in]  _capacity  its size, the terminator included.
 * @param[out] _length    receives the decoded length, whether or not it
 *                        fit.
 * @return D_NET_URL_OK, D_NET_URL_ERROR_ARGUMENT,
 *         D_NET_URL_ERROR_PERCENT, or D_NET_URL_ERROR_BUFFER.
 */
enum d_net_url_error d_net_url_decode(struct d_pack_text _text,
                                      char*              _buffer,
                                      size_t             _capacity,
                                      size_t*            _length);


//==============================================================================
// 5.  SCHEMES AND ENDPOINTS
//==============================================================================
// What a scheme implies, and what a URL names on the network.


// 5.1    Schemes
//------------------------------------------------------------------------------
// schemes -- case-insensitive; the port a registered scheme implies, 0 for
// others, and whether it runs over TLS from its first byte (https, wss,
// ftps, smtps, imaps, pop3s, ldaps); and each error's name
d_net_port           d_net_url_default_port(struct d_pack_text _scheme);
bool                 d_net_url_scheme_is_tls(struct d_pack_text _scheme);
const char*          d_net_url_error_name(enum d_net_url_error _error);

// 5.2    Endpoints
//------------------------------------------------------------------------------
/**
 * @brief The TCP endpoint a URL names: its host, and its explicit port or
 *        its scheme's default.
 *
 * @param[in]  _url  the URL; an IPv6 zone is decoded ("%25eth0" becomes
 *                   "%eth0"), and a registered name's escapes must decode
 *                   to ASCII.
 * @param[out] _out  receives the endpoint.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_INVALID_ARGUMENT for NULL;
 *         D_NET_ERROR_ADDRESS_INVALID without an authority or a host, for
 *         an IPvFuture address, a non-ASCII name, a host over
 *         D_NET_HOST_MAX, or no port at all.
 */
enum d_net_error     d_net_url_endpoint(const struct d_net_url* _url,
                                        struct d_net_endpoint*  _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_URL_H
