/*******************************************************************************
* djinterp [net]                                                    ssh_base64.h
*
* Base64 (RFC 4648 section 4) as SSH uses it: padded in known_hosts and
* public-key files, unpadded in SHA256 fingerprints.
*   Decoding is strict -- no whitespace, no characters outside the
* alphabet, no padding where none belongs, and no stray bits in a final
* partial group -- so every byte string has exactly one accepted
* encoding.
*
*
* path:      /inc/djinterp/net/ssh/ssh_base64.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BASE64
    ------
    1.  Encoding
    2.  Decoding
*/

#ifndef DJINTERP_NET_SSH_SSH_BASE64_H
#define DJINTERP_NET_SSH_SSH_BASE64_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  BASE64
//==============================================================================


// 1.1    Encoding
//------------------------------------------------------------------------------
/**
 * @brief Computes the length of the Base64 encoding of `_size` bytes, not
 *        counting the terminating NUL.
 *
 * @param[in] _size     the length of the data.
 * @param[in] _padding  `true` to count trailing `=` padding.
 * @return the length, or 0 if it would overflow a `size_t`.
 */
D_NODISCARD size_t
d_ssh_base64_encoded_size(size_t _size,
                          bool   _padding);
/**
 * @brief Encodes bytes as NUL-terminated Base64 (RFC 4648, standard
 *        alphabet).
 *
 * @param[in]  _data      the bytes; may be `NULL` when `_size` is 0.
 * @param[in]  _size      their length.
 * @param[in]  _padding   `true` for trailing `=` padding, as in key files;
 *                        `false` for none, as in fingerprints.
 * @param[out] _text      receives the encoding and its NUL.
 * @param[in]  _capacity  the size of `_text`, at least
 *                        d_ssh_base64_encoded_size() + 1.
 * @return `D_SSH_OK`; or `D_SSH_ERR_ARGUMENT` if a pointer is `NULL` or
 *         `_capacity` is too small, leaving `_text` empty where it can.
 */
D_NODISCARD enum d_ssh_status
d_ssh_base64_encode(const void* _data,
                    size_t      _size,
                    bool        _padding,
                    char*       _text,
                    size_t      _capacity);

// 1.2    Decoding
//------------------------------------------------------------------------------
/**
 * @brief Decodes Base64 strictly: padded or unpadded, but in canonical form.
 *
 * @note refuses characters outside the alphabet, whitespace, misplaced or
 *       excess padding, a lone final character, and nonzero unused bits in
 *       the last character -- as OpenSSH does, so each byte string has
 *       exactly one accepted encoding.
 *
 * @param[in]  _text      the encoding; need not be NUL-terminated.
 * @param[in]  _length    its length in characters.
 * @param[out] _data      receives the bytes.
 * @param[in]  _capacity  the size of `_data`; `_length` / 4 * 3 + 2 always
 *                        suffices.
 * @param[out] _size      receives the number of bytes decoded.
 * @return `D_SSH_OK`; `D_SSH_ERR_FORMAT` if the text is not canonical Base64;
 *         or `D_SSH_ERR_ARGUMENT` if a pointer is `NULL` or `_capacity` is
 *         too small.
 */
D_NODISCARD enum d_ssh_status
d_ssh_base64_decode(const char* _text,
                    size_t      _length,
                    void*       _data,
                    size_t      _capacity,
                    size_t*     _size);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_BASE64_H
