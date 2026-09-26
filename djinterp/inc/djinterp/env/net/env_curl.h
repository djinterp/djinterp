/*******************************************************************************
* djinterp [env]                                                      env_curl.h
*
* djinterp libcurl environment detection.
*   The single home for all compile-time detection tied to cURL / libcurl,
* expressed through the D_ENV_CURL_* interface:
*     - header availability (<curl/curl.h>, <curl/curlver.h>)          [1]
*     - version extraction from the packed LIBCURL_VERSION_NUM, and the
*       D_ENV_CURL_VERSION_AT_LEAST comparison helper                   [2]
*     - version-gated API-surface availability (URL, MIME, option, and
*       typed-header APIs)                                              [3]
*     - a rolled-up availability gate, D_ENV_CURL_AVAILABLE             [4]
*   Detection versus runtime features: the API flags derive from the header
* version, so they are reliable at compile time. Whether a particular libcurl
* build supports a wire feature (HTTP/2, a given TLS backend, an
* authentication scheme) depends on how that binary was compiled, and only
* curl_version_info() can tell, at runtime. This header does not guess: it
* answers "is this API present to call?", not "will it succeed?".
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_CURL_* macro to override detection, for example to pin a version when
* cross-compiling against headers the probing compiler cannot see.
*   It requires env_net.h, for D_ENV_NET_HAS_INCLUDE and D_ENV_NET_CAN_TCP,
* and includes it itself. It is an opt-in module and may be included directly.
*
* path:      /inc/djinterp/env/net/env_curl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.16
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  HEADER AVAILABILITY
    -------------------
    1.  Public headers
         1.  D_ENV_CURL_HAS_HEADER
         2.  D_ENV_CURL_HAS_CURLVER_HEADER
2.  VERSION EXTRACTION
    ------------------
    1.  Version probe
         1.  D_CFG_ENV_CURL_PROBE_VERSION
         2.  <curl/curlver.h>
    2.  Version components
         1.  D_ENV_CURL_VERSION_NUM
         2.  D_ENV_CURL_VERSION_STRING
         3.  D_ENV_CURL_VERSION_MAJOR
         4.  D_ENV_CURL_VERSION_MINOR
         5.  D_ENV_CURL_VERSION_PATCH
         6.  D_ENV_CURL_VERSION_AT_LEAST
3.  VERSION-GATED API SURFACE
    -------------------------
    1.  API families
         1.  D_ENV_CURL_HAS_MIME_API
         2.  D_ENV_CURL_HAS_URL_API
         3.  D_ENV_CURL_HAS_OPTION_API
         4.  D_ENV_CURL_HAS_HEADER_API
4.  AVAILABILITY SUMMARY
    --------------------
    1.  Rolled-up availability
         1.  D_ENV_CURL_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_ENV_CURL_H
#define DJINTERP_ENV_NET_ENV_CURL_H 1

// djinterp
#include "./env_net.h"  // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP


//==============================================================================
// 1.  HEADER AVAILABILITY
//==============================================================================


// 1.1    Public headers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_CURL_HAS_HEADER
//   feature: detect if the primary public header <curl/curl.h> is includable.
#ifndef D_ENV_CURL_HAS_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<curl/curl.h>)
        #define D_ENV_CURL_HAS_HEADER 1
    #else
        #define D_ENV_CURL_HAS_HEADER 0
    #endif
#endif  // D_ENV_CURL_HAS_HEADER

// 1.1.2
// D_ENV_CURL_HAS_CURLVER_HEADER
//   feature: detect if the standalone version header <curl/curlver.h> is
// includable (it carries LIBCURL_VERSION / LIBCURL_VERSION_NUM and pulls in no
// declarations or linkage).
#ifndef D_ENV_CURL_HAS_CURLVER_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<curl/curlver.h>)
        #define D_ENV_CURL_HAS_CURLVER_HEADER 1
    #else
        #define D_ENV_CURL_HAS_CURLVER_HEADER 0
    #endif
#endif  // D_ENV_CURL_HAS_CURLVER_HEADER


//==============================================================================
// 2.  VERSION EXTRACTION
//==============================================================================
// libcurl packs its version into the 24-bit LIBCURL_VERSION_NUM, as 0xMMNNPP
// (major, minor, patch). When the version header is present it is included
// and the number decomposed; otherwise the version reads 0, unknown, and
// every D_ENV_CURL_VERSION_AT_LEAST test is false.


// 2.1    Version probe
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_CURL_PROBE_VERSION
//   config: set 0 before inclusion to suppress the automatic
// <curl/curlver.h> include (e.g. to keep detection from touching any curl
// header). Version macros then fall back to "unknown" unless pre-defined.
#ifndef D_CFG_ENV_CURL_PROBE_VERSION
    #define D_CFG_ENV_CURL_PROBE_VERSION 1
#endif  // D_CFG_ENV_CURL_PROBE_VERSION

// 2.1.2
// <curl/curlver.h>
//   include: included when D_CFG_ENV_CURL_PROBE_VERSION is 1 and the header
// exists. It defines only the version macros, and brings in no declarations
// or linkage.
#if ( (D_CFG_ENV_CURL_PROBE_VERSION) &&                                        \
      (D_ENV_CURL_HAS_CURLVER_HEADER) )
    // curl
    #include <curl/curlver.h>  // LIBCURL_VERSION_NUM, LIBCURL_VERSION
#endif

// 2.2    Version components
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_CURL_VERSION_NUM
//   feature: the packed 0xMMNNPP version number, or 0 when unknown.
#ifndef D_ENV_CURL_VERSION_NUM
    #ifdef LIBCURL_VERSION_NUM
        #define D_ENV_CURL_VERSION_NUM LIBCURL_VERSION_NUM
    #else
        #define D_ENV_CURL_VERSION_NUM 0
    #endif  // LIBCURL_VERSION_NUM
#endif  // D_ENV_CURL_VERSION_NUM

// 2.2.2
// D_ENV_CURL_VERSION_STRING
//   feature: the dotted version string, or "unknown".
#ifndef D_ENV_CURL_VERSION_STRING
    #ifdef LIBCURL_VERSION
        #define D_ENV_CURL_VERSION_STRING LIBCURL_VERSION
    #else
        #define D_ENV_CURL_VERSION_STRING "unknown"
    #endif  // LIBCURL_VERSION
#endif  // D_ENV_CURL_VERSION_STRING

// 2.2.3
// D_ENV_CURL_VERSION_MAJOR
//   feature: the major component of the detected version (0 when unknown).
#ifndef D_ENV_CURL_VERSION_MAJOR
    #define D_ENV_CURL_VERSION_MAJOR ((D_ENV_CURL_VERSION_NUM >> 16) & 0xFF)
#endif  // D_ENV_CURL_VERSION_MAJOR

// 2.2.4
// D_ENV_CURL_VERSION_MINOR
//   feature: the minor component of the detected version (0 when unknown).
#ifndef D_ENV_CURL_VERSION_MINOR
    #define D_ENV_CURL_VERSION_MINOR ((D_ENV_CURL_VERSION_NUM >> 8) & 0xFF)
#endif  // D_ENV_CURL_VERSION_MINOR

// 2.2.5
// D_ENV_CURL_VERSION_PATCH
//   feature: the patch component of the detected version (0 when unknown).
#ifndef D_ENV_CURL_VERSION_PATCH
    #define D_ENV_CURL_VERSION_PATCH (D_ENV_CURL_VERSION_NUM & 0xFF)
#endif  // D_ENV_CURL_VERSION_PATCH

// 2.2.6
// D_ENV_CURL_VERSION_AT_LEAST
//   macro: evaluates to 1 if the detected libcurl version is greater than or
// equal to major.minor.patch. False when the version is unknown (0), so it
// doubles as an "is libcurl present at this version" test.
#ifndef D_ENV_CURL_VERSION_AT_LEAST
    #define D_ENV_CURL_VERSION_AT_LEAST(major, minor, patch)                   \
        ( D_ENV_CURL_VERSION_NUM >=                                            \
          ( ((major) << 16) | ((minor) << 8) | (patch) ) )
#endif  // D_ENV_CURL_VERSION_AT_LEAST


//==============================================================================
// 3.  VERSION-GATED API SURFACE
//==============================================================================
// The notable additive C APIs, each keyed to the release that introduced it.
// They are reliable at compile time, since an installed header of a given
// version declares the symbol, but say nothing about runtime success.


// 3.1    API families
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_CURL_HAS_MIME_API
//   feature: the curl_mime_* multipart API (curl_mime_init et al.),
// introduced in libcurl 7.56.0.
#ifndef D_ENV_CURL_HAS_MIME_API
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 56, 0)
        #define D_ENV_CURL_HAS_MIME_API 1
    #else
        #define D_ENV_CURL_HAS_MIME_API 0
    #endif
#endif  // D_ENV_CURL_HAS_MIME_API

// 3.1.2
// D_ENV_CURL_HAS_URL_API
//   feature: the curl_url* URL parsing API (CURLU handle), introduced in
// libcurl 7.62.0.
#ifndef D_ENV_CURL_HAS_URL_API
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 62, 0)
        #define D_ENV_CURL_HAS_URL_API 1
    #else
        #define D_ENV_CURL_HAS_URL_API 0
    #endif
#endif  // D_ENV_CURL_HAS_URL_API

// 3.1.3
// D_ENV_CURL_HAS_OPTION_API
//   feature: the curl_easy_option_* introspection API (by_name / by_id / next),
// introduced in libcurl 7.73.0.
#ifndef D_ENV_CURL_HAS_OPTION_API
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 73, 0)
        #define D_ENV_CURL_HAS_OPTION_API 1
    #else
        #define D_ENV_CURL_HAS_OPTION_API 0
    #endif
#endif  // D_ENV_CURL_HAS_OPTION_API

// 3.1.4
// D_ENV_CURL_HAS_HEADER_API
//   feature: the typed response-header API (curl_easy_header /
// curl_easy_nextheader), introduced in libcurl 7.83.0.
#ifndef D_ENV_CURL_HAS_HEADER_API
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 83, 0)
        #define D_ENV_CURL_HAS_HEADER_API 1
    #else
        #define D_ENV_CURL_HAS_HEADER_API 0
    #endif
#endif  // D_ENV_CURL_HAS_HEADER_API


//==============================================================================
// 4.  AVAILABILITY SUMMARY
//==============================================================================


// 4.1    Rolled-up availability
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_CURL_AVAILABLE
//   feature: 1 when libcurl can actually be used here -- its header is present
// AND the platform has a TCP transport with name resolution
// (D_ENV_NET_CAN_TCP). This is the gate the libcurl foundational module
// (net/curl/curl.hpp) keys on.
#ifndef D_ENV_CURL_AVAILABLE
    #if ( (D_ENV_CURL_HAS_HEADER) &&                                           \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_CURL_AVAILABLE 1
    #else
        #define D_ENV_CURL_AVAILABLE 0
    #endif
#endif  // D_ENV_CURL_AVAILABLE


#endif  // DJINTERP_ENV_NET_ENV_CURL_H
