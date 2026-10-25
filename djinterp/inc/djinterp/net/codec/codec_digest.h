/*******************************************************************************
* djinterp [net]                                                  codec_digest.h
*
* Message digests for net's protocols: MD5, SHA-1 and SHA-256, HMAC over each,
* and the two operations a digest of a secret needs.
*   One implementation serves every module that hashes: SHA-256 for
* certificate and host-key fingerprints (ssl, ssh), HMAC-SHA1 for OpenSSH's
* hashed known_hosts entries (ssh), MD5 for APOP (pop) and HMAC-MD5 for
* CRAM-MD5 (smtp). It is written here rather than taken from a crypto
* library, so every module digests the same way whichever TLS or SSH engine
* is compiled, or none. It supplies
*     - the digest functions, their sizes, and the running state          [1]
*     - incremental and one-shot digests, and HMAC (RFC 2104)             [2]
*     - comparison in constant time, and wiping the compiler cannot drop  [3]
*   MD5 and SHA-1 no longer resist collisions; they are here only because
* the protocols above require them.
*   Nothing allocates. Running time depends on the lengths of the inputs, not
* on their values, and a digest wipes its own state when it finishes, so a
* secret it has read does not stay behind in it.
*   The running state counts what it has digested in a 64-bit integer, so
* the header needs that type, and compiles to nothing where the build has
* none.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/net/codec/codec_digest.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Sizes
         1.  D_NET_DIGEST_MD5_SIZE
         2.  D_NET_DIGEST_SHA1_SIZE
         3.  D_NET_DIGEST_SHA256_SIZE
         4.  D_NET_DIGEST_MAX_SIZE
         5.  D_NET_DIGEST_BLOCK_SIZE
    2.  Functions
         1.  d_net_digest_kind
    3.  The running state
         1.  d_net_digest
2.  DIGESTS
    -------
    1.  Sizes
    2.  Incremental digests
    3.  One-shot digests
    4.  HMAC
3.  SECRETS
    -------
    1.  Comparison
    2.  Wiping
*/

#ifndef DJINTERP_NET_CODEC_CODEC_DIGEST_H
#define DJINTERP_NET_CODEC_CODEC_DIGEST_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_bytes
#include "../net_common.h"              // D_INTERNAL_NET_HAS_INT64
#include "../../config/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint8_t, uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, and compiles to
// nothing where the build has none -- ISO strict C++98 wherever int64_t is
// `long long`: a 32-bit target, 64-bit Windows or macOS (the owner's ruling of
// 2026.10.03 on round 3's question 1, (a)). The answer is net_common.h's, taken
// once per translation unit, so that net's gated headers agree.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// The three functions, the size of what each produces, and the state a digest
// keeps between calls.


// 1.1    Sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_NET_DIGEST_MD5_SIZE
//   constant: the octets of an MD5 digest (RFC 1321).
#define D_NET_DIGEST_MD5_SIZE 16

// 1.1.2
// D_NET_DIGEST_SHA1_SIZE
//   constant: the octets of a SHA-1 digest (FIPS 180-4).
#define D_NET_DIGEST_SHA1_SIZE 20

// 1.1.3
// D_NET_DIGEST_SHA256_SIZE
//   constant: the octets of a SHA-256 digest (FIPS 180-4).
#define D_NET_DIGEST_SHA256_SIZE 32

// 1.1.4
// D_NET_DIGEST_MAX_SIZE
//   constant: the octets of the largest digest, enough for any of them.
#define D_NET_DIGEST_MAX_SIZE 32

// 1.1.5
// D_NET_DIGEST_BLOCK_SIZE
//   constant: the octets each function compresses at a time, the same for all
// three; HMAC pads its key to this length.
#define D_NET_DIGEST_BLOCK_SIZE 64

// 1.2    Functions
//------------------------------------------------------------------------------
// 1.2.1
// d_net_digest_kind
//   enum: a digest function. D_NET_DIGEST_NONE names none: it marks a state
// that cannot be used, which is what a zeroed state holds.
enum d_net_digest_kind
{
    D_NET_DIGEST_NONE   = 0,  // no function
    D_NET_DIGEST_MD5    = 1,  // RFC 1321; 16 octets
    D_NET_DIGEST_SHA1   = 2,  // FIPS 180-4; 20 octets
    D_NET_DIGEST_SHA256 = 3   // FIPS 180-4; 32 octets
};

// 1.3    The running state
//------------------------------------------------------------------------------
// 1.3.1
// d_net_digest
//   struct: a digest in progress. Its members are the implementation's: start
// one with d_net_digest_init, and read nothing from it directly.
struct d_net_digest
{
    enum d_net_digest_kind kind;      // the function; NONE when unusable
    uint32_t               state[8];  // the chaining value
    uint64_t               length;    // octets digested so far
    size_t                 used;      // octets waiting in `block`
    uint8_t                block[D_NET_DIGEST_BLOCK_SIZE];  // a partial block
};


//==============================================================================
// 2.  DIGESTS
//==============================================================================
// A digest of data in pieces, of one buffer, and an HMAC. A digest's value is
// written to the caller's buffer, d_net_digest_size() octets of it.


// 2.1    Sizes
//------------------------------------------------------------------------------
/**
 * @brief The size of a function's digest.
 *
 * @param[in] _kind  the function.
 * @return the size in octets: 16, 20 or 32; or 0 for D_NET_DIGEST_NONE, or a
 *         value that names no function.
 */
D_NODISCARD size_t
d_net_digest_size(enum d_net_digest_kind _kind);

// 2.2    Incremental digests
//------------------------------------------------------------------------------
// A digest of data that arrives in pieces: d_net_digest_init, any number of
// calls to d_net_digest_update, then d_net_digest_final, whose result is the
// one to check. An argument refused along the way leaves the state unusable,
// and d_net_digest_final reports it, so no refusal passes unnoticed.
/**
 * @brief Starts a digest.
 *
 * @param[out] _digest  the state to start; `NULL` is ignored.
 * @param[in]  _kind    the function to compute.
 * @post `_digest` is ready for d_net_digest_update; or, if `_kind` names no
 *       function, unusable, which d_net_digest_final reports.
 */
void
d_net_digest_init(struct d_net_digest*   _digest,
                  enum d_net_digest_kind _kind);
/**
 * @brief Adds data to a digest.
 *
 * @param[in,out] _digest  a started digest; `NULL`, or a digest that is not
 *                         usable, is left as it is.
 * @param[in]     _data    the data; may be `NULL` when `_size` is 0.
 * @param[in]     _size    its length in octets.
 * @post `_digest` covers the data; or, if `_data` is `NULL` and `_size` is not
 *       0, `_digest` is unusable, which d_net_digest_final reports.
 */
void
d_net_digest_update(struct d_net_digest* _digest,
                    const void*          _data,
                    size_t               _size);
/**
 * @brief Finishes a digest and writes its value.
 *
 * @param[in,out] _digest  the digest to finish.
 * @param[out]    _out     receives d_net_digest_size() octets.
 * @post `_digest` is wiped, and unusable until it is started again, whether or
 *       not the call succeeded.
 * @return `true` once the value is written; `false`, with nothing written, if
 *         an argument is `NULL` or `_digest` was not usable.
 */
D_NODISCARD bool
d_net_digest_final(struct d_net_digest* _digest,
                   uint8_t*             _out);

// 2.3    One-shot digests
//------------------------------------------------------------------------------
/**
 * @brief Computes the digest of one buffer.
 *
 * @param[in]  _kind  the function.
 * @param[in]  _data  the data; its `data` may be `NULL` when its `size` is 0.
 * @param[out] _out   receives d_net_digest_size(_kind) octets; it may overlap
 *                    `_data`.
 * @return `true` once the value is written; `false`, with nothing written, if
 *         `_kind` names no function, `_out` is `NULL`, or `_data` is `NULL`
 *         with a nonzero size.
 */
D_NODISCARD bool
d_net_digest_compute(enum d_net_digest_kind _kind,
                     struct d_pack_bytes    _data,
                     uint8_t*               _out);

// 2.4    HMAC
//------------------------------------------------------------------------------
/**
 * @brief Computes an HMAC (RFC 2104) over any of the digest functions.
 *
 * @note A key longer than D_NET_DIGEST_BLOCK_SIZE stands in by its digest, as
 *       RFC 2104 section 2 directs; an empty key is allowed. The key, and
 *       everything derived from it, is wiped before the call returns.
 *
 * @param[in]  _kind  the digest function.
 * @param[in]  _key   the key; its `data` may be `NULL` when its `size` is 0.
 * @param[in]  _data  the message; its `data` may be `NULL` when its `size` is
 *                    0.
 * @param[out] _out   receives d_net_digest_size(_kind) octets; it may overlap
 *                    `_key` or `_data`.
 * @return `true` once the MAC is written; `false`, with nothing written, if
 *         `_kind` names no function, `_out` is `NULL`, or a span is `NULL`
 *         with a nonzero size.
 */
D_NODISCARD bool
d_net_hmac(enum d_net_digest_kind _kind,
           struct d_pack_bytes    _key,
           struct d_pack_bytes    _data,
           uint8_t*               _out);


//==============================================================================
// 3.  SECRETS
//==============================================================================
// What handling a digest of a secret takes besides the digest: comparing it
// without revealing where it differs, and erasing it afterwards.


// 3.1    Comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two byte strings of one length, in a time that does not
 *        depend on where they differ.
 *
 * @note For digests, MACs and fingerprints, where returning at the first
 *       difference would tell an attacker how much of a guess was right.
 *
 * @param[in] _a     the first string.
 * @param[in] _b     the second.
 * @param[in] _size  the length of each, in octets.
 * @return `true` if they are equal; `false` if they differ, or if either is
 *         `NULL`.
 */
D_NODISCARD bool
d_net_digest_equal(const void* _a,
                   const void* _b,
                   size_t      _size);

// 3.2    Wiping
//------------------------------------------------------------------------------
/**
 * @brief Zeroes memory through volatile stores, which the compiler may not
 *        remove as it may a memset of memory that is never read again.
 *
 * @param[out] _data  the memory; `NULL` is ignored.
 * @param[in]  _size  its length in octets.
 */
void
d_net_wipe(void*  _data,
           size_t _size);


D_EXTERN_C_END


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "codec_digest.h: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_NET_CODEC_CODEC_DIGEST_H
