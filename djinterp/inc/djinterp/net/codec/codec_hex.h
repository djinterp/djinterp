/*******************************************************************************
* djinterp [net]                                                     codec_hex.h
*
* Hexadecimal for net's protocols: digests rendered and read back (APOP,
* CRAM-MD5, certificate pins), percent-encoding's escapes, and IPv6's groups.
*   It supplies whole strings of octets, encoded and decoded               [1]
* Decoding reads either case, as every protocol here allows; encoding writes
* the case asked for. Single digits, and the flag that chooses their case,
* are net_common.h's, which most of net's modules read.
*
*
* path:      /inc/djinterp/net/codec/codec_hex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  HEX
    ---
    1.  Encoding and decoding
*/

#ifndef DJINTERP_NET_CODEC_CODEC_HEX_H
#define DJINTERP_NET_CODEC_CODEC_HEX_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_bytes, d_pack_text
#include "./codec_common.h"            // d_net_codec_error
#include "../net_common.h"             // d_net_hex_flag, the single digits


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  HEX
//==============================================================================
// Single digits, and the flag that chooses their case, are net_common.h's.


// 1.1    Encoding and decoding
//------------------------------------------------------------------------------
/**
 * @brief Encodes octets as NUL-terminated hex, two digits each, high first.
 *
 * @param[in]  _data      the octets; its `data` may be `NULL` when its `size`
 *                        is 0.
 * @param[in]  _flags     D_NET_HEX_UPPER, or not.
 * @param[out] _text      receives the digits and a NUL; `NULL` with a
 *                        `_capacity` of 0 to measure.
 * @param[in]  _capacity  the size of `_text`, NUL included.
 * @param[out] _length    receives the digits' count, NUL excluded, even when
 *                        they do not fit; may be `NULL`.
 * @return D_NET_CODEC_OK; D_NET_CODEC_ERROR_BUFFER_TOO_SMALL, leaving `_text`
 *         empty where it has room for the NUL; or
 *         D_NET_CODEC_ERROR_INVALID_ARGUMENT for a missing argument, an unknown
 *         flag, or a length too long for a `size_t`.
 */
D_NODISCARD enum d_net_codec_error
d_net_hex_encode(struct d_pack_bytes _data,
                 unsigned int        _flags,
                 char*               _text,
                 size_t              _capacity,
                 size_t*             _length);
/**
 * @brief Decodes hex of either case into octets, two digits each.
 *
 * @param[in]  _text      the digits, an even number of them, and nothing
 *                        else; need not be NUL-terminated.
 * @param[out] _data      receives the octets; `NULL` with a `_capacity` of 0
 *                        to measure.
 * @param[in]  _capacity  the size of `_data`.
 * @param[out] _size      receives the octets' count, even when they do not
 *                        fit; 0 if the text is refused.
 * @return D_NET_CODEC_OK; D_NET_CODEC_ERROR_MALFORMED for an odd count or a
 *         character that is not a digit; D_NET_CODEC_ERROR_BUFFER_TOO_SMALL; or
 *         D_NET_CODEC_ERROR_INVALID_ARGUMENT for a missing argument. Nothing is
 *         written unless the call succeeds.
 */
D_NODISCARD enum d_net_codec_error
d_net_hex_decode(struct d_pack_text _text,
                 void*              _data,
                 size_t             _capacity,
                 size_t*            _size);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CODEC_CODEC_HEX_H
