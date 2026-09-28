/*******************************************************************************
* djinterp [net]                                                          curl.h
*
* The libcurl binding: HTTP and the other URL schemes libcurl speaks, from C.
*   A request -- method, URL, header fields, body -- and a set of options go
* in; the response's status comes back, and its body and header fields flow
* to callbacks as they arrive. One call performs one transfer on its own easy
* handle, blocking until it ends:
*     - status codes, one per way a transfer can fail                [2]
*     - library queries: version, features, global state             [3]
*     - requests and options, with libcurl's defaults made explicit  [4]
*     - transfers: the sink callbacks, the result, and the driver    [5]
*   NO LIBCURL TYPE APPEARS HERE. The header compiles everywhere; where the
* build lacks libcurl (cfg_curl.h), every transfer reports
* D_CURL_STATUS_UNSUPPORTED and the queries report nothing, so a caller
* decides at run time instead of by conditional compilation.
*   Text spans are d_pack_text and byte spans d_pack_bytes, from
* sink_common.h. Every declaration has C linkage.
*   Include this header by its path from the framework root: a build that
* put inc/djinterp/net on its include path would let this file answer for
* libcurl's own <curl/curl.h>.
*
*
* path:      /inc/djinterp/net/curl/curl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Defaults
         1.  D_CURL_USER_AGENT_DEFAULT
         2.  D_CURL_MAX_REDIRECTS_DEFAULT
2.  RESULT STATUS
    -------------
    1.  Status codes
         1.  d_curl_status
    2.  Status queries
3.  LIBRARY
    -------
    1.  Features
         1.  d_curl_feature
    2.  Library queries
    3.  Global state
4.  REQUESTS
    --------
    1.  Request records
         1.  d_curl_header
         2.  d_curl_request
         3.  d_curl_options
    2.  Request operations
5.  TRANSFERS
    ---------
    1.  Sinks and results
         1.  d_curl_body_fn
         2.  d_curl_header_fn
         3.  d_curl_sink
         4.  d_curl_result
    2.  Performing a transfer
*/

#ifndef DJINTERP_NET_CURL_CURL_H
#define DJINTERP_NET_CURL_CURL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_text, d_pack_bytes


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Defaults
//------------------------------------------------------------------------------
// 1.1.1
// D_CURL_USER_AGENT_DEFAULT
//   constant: the User-Agent a request sends when its options name none.
#define D_CURL_USER_AGENT_DEFAULT    "djinterp"

// 1.1.2
// D_CURL_MAX_REDIRECTS_DEFAULT
//   constant: how many redirects d_curl_options_init allows a transfer to
// follow.
#define D_CURL_MAX_REDIRECTS_DEFAULT 30


//==============================================================================
// 2.  RESULT STATUS
//==============================================================================
// How a transfer ended, as seen from the transport: OK means a response
// arrived, whatever its HTTP status. The categories are those of the C++ web
// layer's transport_error, so the two translate one for one; the C binding
// adds INVALID_ARGUMENT and UNSUPPORTED.


// 2.1    Status codes
//------------------------------------------------------------------------------
// 2.1.1
// d_curl_status
//   enum: the result of a transfer or a library call. Values are pinned. The
// less obvious ones:
//     UNSUPPORTED           the build has no libcurl.
//     UNSUPPORTED_PROTOCOL  libcurl does not speak the URL's scheme, or the
//                           URL is malformed.
//     TLS_ERROR             the TLS handshake failed, or the peer failed
//                           verification.
//     WRITE_ERROR           a sink refused data, or libcurl could not
//                           deliver it.
//     UNKNOWN               any other libcurl failure; the result's code
//                           holds libcurl's own number.
enum d_curl_status
{
    D_CURL_STATUS_OK                     = 0,
    D_CURL_STATUS_INVALID_ARGUMENT       = 0x001,
    D_CURL_STATUS_UNSUPPORTED            = 0x002,
    D_CURL_STATUS_UNSUPPORTED_PROTOCOL   = 0x003,
    D_CURL_STATUS_COULD_NOT_RESOLVE_HOST = 0x004,
    D_CURL_STATUS_COULD_NOT_CONNECT      = 0x005,
    D_CURL_STATUS_TIMED_OUT              = 0x006,
    D_CURL_STATUS_TLS_ERROR              = 0x007,
    D_CURL_STATUS_TOO_MANY_REDIRECTS     = 0x008,
    D_CURL_STATUS_WRITE_ERROR            = 0x009,
    D_CURL_STATUS_READ_ERROR             = 0x00A,
    D_CURL_STATUS_CANCELED               = 0x00B,
    D_CURL_STATUS_OUT_OF_MEMORY          = 0x00C,
    D_CURL_STATUS_UNKNOWN                = 0x00D
};

// 2.2    Status queries
//------------------------------------------------------------------------------
// Pure lookups; they never fail. d_curl_status_name gives the status's name
// in lower case ("timed out"), or "invalid status" for a value not listed.
// d_curl_code_message gives libcurl's own text for one of its codes, as
// d_curl_result.code carries them, or NULL without libcurl.
const char* d_curl_status_name(enum d_curl_status _status);
const char* d_curl_code_message(int _code);


//==============================================================================
// 3.  LIBRARY
//==============================================================================
// What the libcurl this program runs with can do. Whether the build has an
// API is fixed at compile time (env_curl.h); whether the library supports a
// feature -- HTTP/2, a TLS backend -- is a property of the library found at
// run time, which these report.


// 3.1    Features
//------------------------------------------------------------------------------
// 3.1.1
// d_curl_feature
//   enum: a capability libcurl reports at run time.
//     SSL         can speak TLS.
//     HTTP2       can speak HTTP/2.
//     HTTP3       can speak HTTP/3.
//     IPV6        can use IPv6.
//     LIBZ        can decode gzip and deflate.
//     BROTLI      can decode Brotli.
//     ZSTD        can decode Zstandard.
//     ASYNCH_DNS  resolves names without blocking the transfer's thread.
//     THREADSAFE  initializes its global state thread-safely.
enum d_curl_feature
{
    D_CURL_FEATURE_SSL        = 1,
    D_CURL_FEATURE_HTTP2      = 2,
    D_CURL_FEATURE_HTTP3      = 3,
    D_CURL_FEATURE_IPV6       = 4,
    D_CURL_FEATURE_LIBZ       = 5,
    D_CURL_FEATURE_BROTLI     = 6,
    D_CURL_FEATURE_ZSTD       = 7,
    D_CURL_FEATURE_ASYNCH_DNS = 8,
    D_CURL_FEATURE_THREADSAFE = 9
};

// 3.2    Library queries
//------------------------------------------------------------------------------
// Pure queries of the libcurl in use; they never fail. d_curl_version gives
// its version text ("8.5.0") and d_curl_version_number its version as
// 0xMMmmpp, or NULL and 0 without libcurl. d_curl_supports reports a
// feature, false for one the build's headers predate and without libcurl.
const char* d_curl_version(void);
uint32_t    d_curl_version_number(void);
bool        d_curl_supports(enum d_curl_feature _feature);

// 3.3    Global state
//------------------------------------------------------------------------------
/**
 * @brief Initializes libcurl's global state: TLS, sockets, and the rest.
 *
 * @note Optional: the first transfer initializes it otherwise. Where
 *       d_curl_supports(D_CURL_FEATURE_THREADSAFE) is false, call this, or
 *       perform a transfer, before starting other threads that use the
 *       binding, as libcurl requires.
 *
 * @post Each successful call is balanced by one d_curl_global_cleanup,
 *       which releases what it acquired; the last frees libcurl's global
 *       state. Without libcurl, d_curl_global_cleanup does nothing.
 * @return D_CURL_STATUS_OK; D_CURL_STATUS_OUT_OF_MEMORY or
 *         D_CURL_STATUS_UNKNOWN if libcurl fails; or
 *         D_CURL_STATUS_UNSUPPORTED without libcurl.
 */
enum d_curl_status d_curl_global_init(void);
void               d_curl_global_cleanup(void);


//==============================================================================
// 4.  REQUESTS
//==============================================================================
// A request and its options are plain records the caller fills and owns,
// borrowed only for the duration of d_curl_perform; libcurl copies what it
// keeps. Options start from d_curl_options_init, which states libcurl's
// defaults explicitly, so weakening one -- verify_tls, say -- is a visible
// assignment.


// 4.1    Request records
//------------------------------------------------------------------------------
// 4.1.1
// d_curl_header
//   struct: one request header field. The name is not empty and holds no
// colon, and neither part holds a line break or a NUL, so no field can end
// early and smuggle in another. An empty value removes a header libcurl
// would send itself (Expect, say), which is libcurl's convention.
struct d_curl_header
{
    struct d_pack_text name;
    struct d_pack_text value;
};

// 4.1.2
// d_curl_request
//   struct: what to send.
//     method        the method token ("GET", "POST", "PATCH", ...), sent as
//                   given; NULL lets libcurl choose: GET, or POST when
//                   there is a body. "HEAD" asks for no body.
//     url           the URL, NUL-terminated.
//     headers       header_count fields, sent in order after libcurl's own.
//     body          the request body, which libcurl copies; empty for none.
struct d_curl_request
{
    const char*                 method;
    const char*                 url;
    const struct d_curl_header* headers;
    size_t                      header_count;
    struct d_pack_bytes         body;
};

// 4.1.3
// d_curl_options
//   struct: how to send it.
//     timeout_ms          the whole transfer's limit; 0 for none.
//     connect_timeout_ms  the connection phase's limit; 0 for libcurl's.
//     follow_redirects    follow Location, up to max_redirects times.
//     verify_tls          verify the peer's certificate and name. Turning
//                         it off is insecure.
//     verbose             have libcurl describe the transfer on stderr.
//     accept_encoding     the codings to accept and decode: "" for every
//                         one libcurl supports, NULL for none.
//     user_agent          the User-Agent; NULL for
//                         D_CURL_USER_AGENT_DEFAULT.
//     proxy               the proxy URL; NULL for libcurl's default, which
//                         reads the environment's proxy variables, and ""
//                         for none, whatever the environment says.
struct d_curl_options
{
    long        timeout_ms;
    long        connect_timeout_ms;
    bool        follow_redirects;
    long        max_redirects;
    bool        verify_tls;
    bool        verbose;
    const char* accept_encoding;
    const char* user_agent;
    const char* proxy;
};

// 4.2    Request operations
//------------------------------------------------------------------------------
// d_curl_options_init fills _options with the defaults: no timeouts,
// redirects followed up to D_CURL_MAX_REDIRECTS_DEFAULT, TLS verified, quiet,
// every coding accepted, the default User-Agent, and libcurl's default proxy.
// A NULL _options is ignored.
void d_curl_options_init(struct d_curl_options* _options);


//==============================================================================
// 5.  TRANSFERS
//==============================================================================
// A transfer delivers the response as it arrives: body bytes in whatever
// pieces the network yields, and each header field parsed into a name and a
// value. Where redirects are followed, the header fields of every response
// in the chain are delivered, in order, and only the last response's body.
// A callback that returns false ends the transfer with WRITE_ERROR.


// 5.1    Sinks and results
//------------------------------------------------------------------------------
// 5.1.1
// d_curl_body_fn
//   function pointer: receives _size (at least 1) bytes of response body.
// Returns true to continue the transfer, false to end it.
typedef bool (*d_curl_body_fn)(void*       _context,
                               const void* _data,
                               size_t      _size);

// 5.1.2
// d_curl_header_fn
//   function pointer: receives one response header field, its name as sent
// and its value without surrounding blanks, both borrowed for the call.
// Status lines -- those beginning "HTTP/" -- and the blank line ending a
// header block are not delivered.
// Returns true to continue the transfer, false to end it.
typedef bool (*d_curl_header_fn)(void*              _context,
                                 struct d_pack_text _name,
                                 struct d_pack_text _value);

// 5.1.3
// d_curl_sink
//   struct: where a response goes. A NULL body or header discards what it
// would have received; context goes to both.
struct d_curl_sink
{
    d_curl_body_fn   body;
    d_curl_header_fn header;
    void*            context;
};

// 5.1.4
// d_curl_result
//   struct: how a transfer ended.
//     status       the outcome.
//     code         libcurl's own code for it, 0 for success; see
//                  d_curl_code_message.
//     http_status  the last response's status code, 0 if none arrived --
//                  set even when the transfer then failed, as when a sink
//                  ended it.
struct d_curl_result
{
    enum d_curl_status status;
    int                code;
    long               http_status;
};

// 5.2    Performing a transfer
//------------------------------------------------------------------------------
/**
 * @brief Performs one transfer, blocking until it ends.
 *
 * @note Each call uses its own easy handle, so calls on different threads
 *       may run at once, subject to the note on d_curl_global_init. The
 *       binding never lets libcurl install signal handlers
 *       (CURLOPT_NOSIGNAL), as multi-threaded programs need.
 *
 * @param[in]  _request  the request; borrowed for the call.
 * @param[in]  _options  the options; NULL for d_curl_options_init's.
 * @param[in]  _sink     where the response goes; NULL discards it.
 * @param[out] _out      receives how the transfer ended; may be NULL.
 * @return the transfer's status, as stored in `_out->status`:
 *         D_CURL_STATUS_OK once a response has been received, whatever its
 *         HTTP status; D_CURL_STATUS_INVALID_ARGUMENT for a NULL `_request`
 *         or URL, a header field breaking d_curl_header's rules, or a span
 *         with a NULL pointer and a nonzero size, checked before libcurl is
 *         asked anything; D_CURL_STATUS_UNSUPPORTED without libcurl; or the
 *         category of the transfer's failure.
 */
enum d_curl_status d_curl_perform(const struct d_curl_request* _request,
                                  const struct d_curl_options* _options,
                                  const struct d_curl_sink*    _sink,
                                  struct d_curl_result*        _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CURL_CURL_H
