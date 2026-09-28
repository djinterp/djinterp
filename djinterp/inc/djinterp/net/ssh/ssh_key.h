/*******************************************************************************
* djinterp [net]                                                       ssh_key.h
*
* SSH public keys and randomness.
*   A key blob is a public key in SSH wire format: a string naming its
* type, then the type's fields. known_hosts stores it in Base64, and a
* fingerprint is the unpadded Base64 of its SHA-256 digest, in OpenSSH's
* "SHA256:" form. The random source supplies the salts of hashed
* known_hosts entries.
*
*
* path:      /inc/djinterp/net/ssh/ssh_key.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Sizes
         1.  D_SSH_FINGERPRINT_SIZE
         2.  D_SSH_KEY_TYPE_SIZE
2.  KEYS
    ----
    1.  Key blobs
    2.  Randomness
*/

#ifndef DJINTERP_NET_SSH_SSH_KEY_H
#define DJINTERP_NET_SSH_SSH_KEY_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_SSH_FINGERPRINT_SIZE
//   constant: the bytes a fingerprint needs: "SHA256:", 43 Base64 characters,
// and the terminating NUL.
#define D_SSH_FINGERPRINT_SIZE 51

// 1.1.2
// D_SSH_KEY_TYPE_SIZE
//   constant: the bytes a key-type name may need, NUL included; longer names
// are refused as malformed.
#define D_SSH_KEY_TYPE_SIZE 64


//==============================================================================
// 2.  KEYS
//==============================================================================


// 2.1    Key blobs
//------------------------------------------------------------------------------
/**
 * @brief Reads a key blob's type name, such as "ssh-ed25519".
 *
 * @param[in]  _key       the blob.
 * @param[in]  _size      its length.
 * @param[out] _type      receives the NUL-terminated name.
 * @param[in]  _capacity  the size of `_type`; D_SSH_KEY_TYPE_SIZE suffices.
 * @return `D_SSH_OK`; `D_SSH_ERR_FORMAT` if the blob does not start with a
 *         non-empty, printable name shorter than D_SSH_KEY_TYPE_SIZE; or
 *         `D_SSH_ERR_ARGUMENT` if a pointer is `NULL` or `_capacity` is too
 *         small.
 */
D_NODISCARD enum d_ssh_status
d_ssh_key_type(const unsigned char* _key,
               size_t               _size,
               char*                _type,
               size_t               _capacity);
/**
 * @brief Formats a key blob's SHA-256 fingerprint as OpenSSH prints it:
 *        "SHA256:" and 43 unpadded Base64 characters.
 *
 * @param[in]  _key       the blob; any bytes are hashed.
 * @param[in]  _size      its length; at least 1.
 * @param[out] _text      receives the NUL-terminated fingerprint.
 * @param[in]  _capacity  the size of `_text`, at least
 *                        D_SSH_FINGERPRINT_SIZE.
 * @return `D_SSH_OK`; or `D_SSH_ERR_ARGUMENT` if a pointer is `NULL`,
 *         `_size` is 0, or `_capacity` is too small.
 */
D_NODISCARD enum d_ssh_status
d_ssh_fingerprint(const unsigned char* _key,
                  size_t               _size,
                  char*                _text,
                  size_t               _capacity);

// 2.2    Randomness
//------------------------------------------------------------------------------
/**
 * @brief Fills a buffer from the platform's cryptographically secure random
 *        source (D_ENV_SSH_RANDOM_SOURCE).
 *
 * @param[out] _buffer  the buffer; may be `NULL` when `_size` is 0.
 * @param[in]  _size    its length.
 * @return `D_SSH_OK`; `D_SSH_ERR_UNSUPPORTED` if the platform has no known
 *         source; `D_SSH_ERR_IO` if the source failed; or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_random(void*  _buffer,
             size_t _size);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_KEY_H
