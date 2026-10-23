/*******************************************************************************
* djinterp [net]                                                  codec_base64.h
*
* Base64 (RFC 4648 section 4) for net's protocols, in each variant they use.
*   One codec serves every module that encodes or decodes base64: SASL's,
* padded and strict (smtp); OpenSSH's, padded or not, and canonical, so each
* byte string has one accepted encoding (ssh); PEM's, whose lines break the
* text with whitespace (ssl); and IMAP's modified alphabet, which mailbox
* names use one character at a time (imap). It supplies
*     - the flags that select a variant                                  [1]
*     - lengths, encoding and decoding, and single characters            [2]
*   Nothing allocates. Both directions write to the caller's buffer and report
* the length the output needs even when the buffer is too small; a call with
* no buffer at all measures. A refused input writes nothing.
*
*
* path:      /inc/djinterp/net/codec/codec_base64.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FLAGS
    -----
    1.  Variants
         1.  d_net_base64_flag
2.  BASE64
    ------
    1.  Lengths
    2.  Encoding and decoding
    3.  Single characters
*/

#ifndef DJINTERP_NET_CODEC_CODEC_BASE64_H
#define DJINTERP_NET_CODEC_CODEC_BASE64_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_bytes, d_pack_text
#include "./codec_common.h"            // d_net_codec_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FLAGS
//==============================================================================


// 1.1    Variants
//------------------------------------------------------------------------------
// 1.1.1
// d_net_base64_flag
//   enum: the variants, as bits to combine. With none of them (STANDARD), the
// encoding is padded and the decoding strict: padding required and exact, no
// whitespace, and any unused bits in a final partial group accepted, as RFC
// 4648 section 3.5 allows a decoder to.
enum d_net_base64_flag
{
    D_NET_BASE64_STANDARD  = 0x00u,  // the strict, padded default
    D_NET_BASE64_UNPADDED  = 0x01u,  // encode no '='; decode with or without
    D_NET_BASE64_BLANKS    = 0x02u,  // decode: skip space, tab, CR and LF
    D_NET_BASE64_CANONICAL = 0x04u,  // decode: unused bits must be zero
    D_NET_BASE64_MODIFIED  = 0x08u   // RFC 3501's alphabet: ',' for '/'
};


//==============================================================================
// 2.  BASE64
//==============================================================================
// The flags each function reads are named in its contract; it ignores the
// others, but refuses a bit that names no flag at all.


// 2.1    Lengths
//------------------------------------------------------------------------------
/**
 * @brief The length of the encoding of `_size` octets, its NUL not counted.
 *
 * @param[in] _size   the octets to encode.
 * @param[in] _flags  D_NET_BASE64_UNPADDED, or not.
 * @return four characters for every three octets, and for the last one or
 *         two either four, padded, or one more than their count, unpadded;
 *         or 0 if the length and a NUL after it would overflow a `size_t`.
 */
D_NODISCARD size_t
d_net_base64_encoded_length(size_t       _size,
                            unsigned int _flags);

// 2.2    Encoding and decoding
//------------------------------------------------------------------------------
/**
 * @brief Encodes octets as NUL-terminated base64.
 *
 * @param[in]  _data      the octets; its `data` may be `NULL` when its `size`
 *                        is 0.
 * @param[in]  _flags     D_NET_BASE64_UNPADDED and D_NET_BASE64_MODIFIED are
 *                        read.
 * @param[out] _text      receives the encoding and a NUL; `NULL` with a
 *                        `_capacity` of 0 to measure.
 * @param[in]  _capacity  the size of `_text`, NUL included.
 * @param[out] _length    receives the encoding's length, NUL excluded, even
 *                        when it does not fit; may be `NULL`.
 * @return D_NET_CODEC_OK; D_NET_CODEC_ERROR_BUFFER_TOO_SMALL, leaving `_text`
 *         empty where it has room for the NUL; or
 *         D_NET_CODEC_ERROR_INVALID_ARGUMENT for a missing argument, an unknown
 *         flag, or an encoding too long for a `size_t`.
 */
D_NODISCARD enum d_net_codec_error
d_net_base64_encode(struct d_pack_bytes _data,
                    unsigned int        _flags,
                    char*               _text,
                    size_t              _capacity,
                    size_t*             _length);
/**
 * @brief Decodes base64 into octets.
 *
 * @note Every flag is read. D_NET_BASE64_UNPADDED accepts text with no
 *       padding, as well as text whose padding completes its last group
 *       exactly; without it, padding is required. A final group of one
 *       character, which holds no whole octet, is always refused.
 *
 * @param[in]  _text      the encoding; need not be NUL-terminated, and its
 *                        `data` may be `NULL` when its `length` is 0.
 * @param[in]  _flags     the variant.
 * @param[out] _data      receives the octets, with no NUL after them; `NULL`
 *                        with a `_capacity` of 0 to measure.
 * @param[in]  _capacity  the size of `_data`.
 * @param[out] _size      receives the octets' count, even when they do not
 *                        fit; 0 if the text is refused.
 * @return D_NET_CODEC_OK; D_NET_CODEC_ERROR_MALFORMED for text that is not in
 *         the variant; D_NET_CODEC_ERROR_BUFFER_TOO_SMALL; or
 *         D_NET_CODEC_ERROR_INVALID_ARGUMENT for a missing argument or an
 *         unknown flag. Nothing is written unless the call succeeds.
 */
D_NODISCARD enum d_net_codec_error
d_net_base64_decode(struct d_pack_text _text,
                    unsigned int       _flags,
                    void*              _data,
                    size_t             _capacity,
                    size_t*            _size);

// 2.3    Single characters
//------------------------------------------------------------------------------
// single characters -- for encodings that are not of octets, such as IMAP's
// mailbox names, which carry UTF-16 in modified base64; only
// D_NET_BASE64_MODIFIED is read.
//   d_net_base64_digit gives the character for a six-bit value, or '\0' for a
// value past 63; d_net_base64_value gives a character's six-bit value, or -1
// for a character outside the alphabet, '=' included.
D_NODISCARD char
d_net_base64_digit(unsigned int _sextet,
                   unsigned int _flags);
D_NODISCARD int
d_net_base64_value(char         _c,
                   unsigned int _flags);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CODEC_CODEC_BASE64_H
