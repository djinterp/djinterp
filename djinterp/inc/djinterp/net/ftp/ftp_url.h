/*******************************************************************************
* djinterp [net]                                                       ftp_url.h
*
* FTP URLs (RFC 1738, RFC 3986): ftp://, ftps://, and ftpes://.
*   Splits a URL into scheme, credentials, host, port, path, and ;type= code
* without decoding it, and percent-decodes and -encodes its parts, refusing
* bytes that would truncate a name or inject a command.
*
*
* path:      /inc/djinterp/net/ftp/ftp_url.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  URLs
         1.  d_ftp_scheme
         2.  d_ftp_url
2.  URLS
    ----
    1.  Schemes
    2.  Parsing
    3.  Percent-encoding
*/

#ifndef DJINTERP_NET_FTP_FTP_URL_H
#define DJINTERP_NET_FTP_FTP_URL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint16_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_span, d_ftp_error, d_ftp_buffer
#include "./ftp_security.h"    // d_ftp_security


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_url.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    URLs
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_scheme
//   enum: the URL schemes that name an FTP resource.
enum d_ftp_scheme
{
    D_FTP_SCHEME_NONE = 0,  // not an FTP URL
    D_FTP_SCHEME_FTP,       // ftp://: plain FTP (RFC 1738)
    D_FTP_SCHEME_FTPS,      // ftps://: implicit TLS on port 990
    D_FTP_SCHEME_FTPES      // ftpes://: explicit TLS, a client convention
};

// 1.1.2
// d_ftp_url
//   struct: a parsed ftp:// URL. Every span points into the parsed text and
// is still percent-encoded; decode it with d_ftp_percent_decode() before use.
struct d_ftp_url
{
    enum d_ftp_scheme scheme;        // the scheme
    struct d_ftp_span user;          // user name, if has_user
    struct d_ftp_span password;      // password, if has_password
    struct d_ftp_span host;          // host; an IPv6 literal's brackets go
    struct d_ftp_span path;          // path after the authority's '/'
    uint16_t          port;          // explicit port, if has_port
    char              type_code;     // ';type=' code: 'a', 'i', 'd', or 0
    bool              has_user;      // a user name was given, maybe empty
    bool              has_password;  // a ':' followed the user name
    bool              has_port;      // an explicit port was given
    bool              ipv6_literal;  // the host was written in brackets
};


//==============================================================================
// 2.  URLS
//==============================================================================


// 2.1    Schemes
//------------------------------------------------------------------------------
// The port a scheme implies (990 for ftps, 21 otherwise, 0 for none), and the
// control-connection security it asks for.
uint16_t            d_ftp_scheme_default_port(enum d_ftp_scheme _scheme);
enum d_ftp_security d_ftp_scheme_security(enum d_ftp_scheme _scheme);

// 2.2    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Parses an ftp://, ftps://, or ftpes:// URL (RFC 1738 3.2).
 *
 * The form is scheme://[user[:password]@]host[:port][/path][;type=X], where
 * X is a, i, or d. The credentials end at the authority's last '@'. Control
 * characters and spaces are refused anywhere, since they must be
 * percent-encoded.
 *
 * @param[in]  _text   the URL.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the parts, as encoded spans into `_text`.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_MALFORMED, or
 *         D_FTP_ERROR_UNSUPPORTED for another scheme.
 */
enum d_ftp_error    d_ftp_url_parse(const char*       _text,
                                    size_t            _length,
                                    struct d_ftp_url* _out);
/**
 * @brief Returns a URL's explicit port, or its scheme's default.
 *
 * @param[in] _url the parsed URL.
 * @return the port; 0 for a NULL `_url`.
 */
uint16_t            d_ftp_url_port(const struct d_ftp_url* _url);

// 2.3    Percent-encoding
//------------------------------------------------------------------------------
/**
 * @brief Appends the percent-decoded form of a URL part.
 *
 * Decoded NUL, CR, and LF bytes are refused: in a path they would truncate
 * the name or inject a command into the control connection.
 *
 * @param[in]     _text   the encoded text.
 * @param[in]     _length its length in bytes.
 * @param[in,out] _out    the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_MALFORMED for
 *         a bad or forbidden escape, or D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error    d_ftp_percent_decode(const char*          _text,
                                         size_t               _length,
                                         struct d_ftp_buffer* _out);
/**
 * @brief Appends the percent-encoded form of a URL part, escaping all but
 *        RFC 3986's unreserved characters.
 *
 * @param[in]     _text         the raw text.
 * @param[in]     _length       its length in bytes.
 * @param[in]     _keep_slashes true to leave '/' unescaped, for paths.
 * @param[in,out] _out          the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error    d_ftp_percent_encode(const char*          _text,
                                         size_t               _length,
                                         bool                 _keep_slashes,
                                         struct d_ftp_buffer* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_URL_H
