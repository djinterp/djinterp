/*******************************************************************************
* djinterp [net]                                                   ftp_feature.h
*
* FTP feature negotiation (RFC 2389).
*   Reads a FEAT reply into capability flags, including the parameters that
* AUTH, REST, MODE, MLST, and LANG carry.
*
*
* path:      /inc/djinterp/net/ftp/ftp_feature.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Features
         1.  d_ftp_feature
         2.  d_ftp_features
2.  FEATURES
    --------
    1.  Parsing
*/

#ifndef DJINTERP_NET_FTP_FTP_FEATURE_H
#define DJINTERP_NET_FTP_FTP_FEATURE_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_span, d_ftp_error


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_feature.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Features
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_feature
//   enum: bit flags for the FEAT capabilities this module recognizes.
enum d_ftp_feature
{
    D_FTP_FEATURE_NONE        = 0,
    D_FTP_FEATURE_EPRT        = 0x00001,  // RFC 2428
    D_FTP_FEATURE_EPSV        = 0x00002,  // RFC 2428
    D_FTP_FEATURE_MDTM        = 0x00004,  // RFC 3659
    D_FTP_FEATURE_SIZE        = 0x00008,  // RFC 3659
    D_FTP_FEATURE_REST_STREAM = 0x00010,  // RFC 3659: REST in stream mode
    D_FTP_FEATURE_MLST        = 0x00020,  // RFC 3659: MLST and MLSD
    D_FTP_FEATURE_TVFS        = 0x00040,  // RFC 3659: '/'-separated paths
    D_FTP_FEATURE_UTF8        = 0x00080,  // RFC 2640
    D_FTP_FEATURE_LANG        = 0x00100,  // RFC 2640
    D_FTP_FEATURE_AUTH_TLS    = 0x00200,  // RFC 4217
    D_FTP_FEATURE_AUTH_SSL    = 0x00400,  // pre-RFC 4217 servers
    D_FTP_FEATURE_PBSZ        = 0x00800,  // RFC 2228
    D_FTP_FEATURE_PROT        = 0x01000,  // RFC 2228
    D_FTP_FEATURE_CCC         = 0x02000,  // RFC 2228
    D_FTP_FEATURE_HOST        = 0x04000,  // RFC 7151
    D_FTP_FEATURE_MFMT        = 0x08000,  // draft: set modification time
    D_FTP_FEATURE_PRET        = 0x10000,  // non-standard
    D_FTP_FEATURE_CLNT        = 0x20000,  // non-standard
    D_FTP_FEATURE_MODE_Z      = 0x40000   // draft: deflate transfers
};

// 1.1.2
// d_ftp_features
//   struct: a parsed FEAT reply. The spans point into the parsed text.
struct d_ftp_features
{
    uint32_t          flags;       // D_FTP_FEATURE_* bits
    struct d_ftp_span mlst_facts;  // the MLST argument, e.g. "type*;size*;"
    struct d_ftp_span languages;   // the LANG argument, e.g. "EN*;FR"
};


//==============================================================================
// 2.  FEATURES
//==============================================================================


// 2.1    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Reads the capabilities from the text of a 211 reply to FEAT.
 *
 * Feature names match case-insensitively; lines that name no known feature,
 * including "Features:" and "End", are ignored, as unknown features must be
 * (RFC 2389 3.2).
 *
 * @param[in]  _text   the reply text, as d_ftp_reply_parser_feed() gives it.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the capabilities; its spans point into `_text`.
 * @return D_FTP_OK or D_FTP_ERROR_INVALID_ARGUMENT.
 */
enum d_ftp_error d_ftp_features_parse(const char*            _text,
                                      size_t                 _length,
                                      struct d_ftp_features* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_FEATURE_H
