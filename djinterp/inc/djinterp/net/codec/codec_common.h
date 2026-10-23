/*******************************************************************************
* djinterp [net]                                                  codec_common.h
*
* The vocabulary net's codec units share: the errors their encoders and
* decoders report, and how they map onto net.h's.
*   An encoder or decoder writes to the caller's buffer and reports the length
* its output needs, even when the buffer is too small, so a first call with no
* buffer can size one and a second fill it. The digests report no error
* code: they refuse only missing arguments, and say so with a bool.
*
*
* path:      /inc/djinterp/net/codec/codec_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ERRORS
    ------
    1.  Error codes
         1.  d_net_codec_error
    2.  Error queries
*/

#ifndef DJINTERP_NET_CODEC_CODEC_COMMON_H
#define DJINTERP_NET_CODEC_CODEC_COMMON_H 1

// djinterp
#include "../../c/djinterp.h"  // framework root
#include "../net_common.h"      // d_net_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ERRORS
//==============================================================================


// 1.1    Error codes
//------------------------------------------------------------------------------
// 1.1.1
// d_net_codec_error
//   enum: the outcome of an encode or a decode. With BUFFER_TOO_SMALL, the
// length the output needs is still reported. The names are spelled as the
// protocol modules' own errors spell the same conditions.
enum d_net_codec_error
{
    D_NET_CODEC_OK                     = 0,  // done; the output is written
    D_NET_CODEC_ERROR_INVALID_ARGUMENT = 1,  // the caller misused the call
    D_NET_CODEC_ERROR_MALFORMED        = 2,  // the input is not in the encoding
    D_NET_CODEC_ERROR_BUFFER_TOO_SMALL = 3   // the output does not fit
};

// 1.2    Error queries
//------------------------------------------------------------------------------
/**
 * @brief Maps an error onto d_net_error, by the rules net.h gives with it.
 *
 * @param[in] _error  the error.
 * @return the d_net_error it maps to; D_NET_ERROR_UNKNOWN for a value
 *         outside the enum.
 */
enum d_net_error d_net_codec_error_to_net(enum d_net_codec_error _error);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CODEC_CODEC_COMMON_H
