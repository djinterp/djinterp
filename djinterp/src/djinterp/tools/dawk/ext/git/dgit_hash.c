/*******************************************************************************
* djinterp [tools]                                                   dgit_hash.c
*
* Definitions for the hashes of dgit_internal.h: SHA-1 and SHA-256.
*   Both as FIPS 180-4 has them, on 64-byte blocks: bytes wait in the block
* until it is full, a full block is compressed into the state, and the end
* pads with a 1 bit, zeros, and the length in bits, big-endian. They differ
* in their states, their schedules and their rounds, so each has its own
* compression; the feeding and the padding are shared. Git hashes SHA-1
* with collision detection, which changes nothing but for a crafted
* collision, which this does not look for.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_hash.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./dgit_internal.h"  // corresponding header
// std
#include <string.h>  // memcpy, memmove, memset
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // uint64_t


// d_internal_sha256_k
//   constant: SHA-256's round constants, the first 32 bits of the
//   fractional parts of the cube roots of the first 64 primes.
static const uint32_t d_internal_sha256_k[64] =
{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};


/*
d_internal_rotl, d_internal_rotr, d_internal_big
  A word rotated left or right, and four bytes read big-endian.
*/
static uint32_t
d_internal_rotl(
    uint32_t _word,
    unsigned _by
)
{
    return (uint32_t)((_word << _by) | (_word >> (32u - _by)));
}


static uint32_t
d_internal_rotr(
    uint32_t _word,
    unsigned _by
)
{
    return (uint32_t)((_word >> _by) | (_word << (32u - _by)));
}


static uint32_t
d_internal_big(
    const uint8_t* _bytes
)
{
    return ((uint32_t)_bytes[0] << 24) |
           ((uint32_t)_bytes[1] << 16) |
           ((uint32_t)_bytes[2] << 8)  |
           (uint32_t)_bytes[3];
}


/*
d_internal_sha1_round
  The function and constant of SHA-1's round `_round`, applied: f(b, c, d)
plus k, by the round's quarter.
*/
static uint32_t
d_internal_sha1_round(
    unsigned _round,
    uint32_t _b,
    uint32_t _c,
    uint32_t _d
)
{
    // choose, parity, majority, parity
    if (_round < 20u)
    {
        return ((_b & _c) | (~_b & _d)) + 0x5a827999u;
    }
    else if (_round < 40u)
    {
        return (_b ^ _c ^ _d) + 0x6ed9eba1u;
    }
    else if (_round < 60u)
    {
        return ((_b & _c) | (_b & _d) | (_c & _d)) + 0x8f1bbcdcu;
    }

    return (_b ^ _c ^ _d) + 0xca62c1d6u;
}


/*
d_internal_sha1_block
  One block compressed into SHA-1's state: the schedule of 80 words, then
80 rounds.
*/
static void
d_internal_sha1_block(
    uint32_t*      _state,
    const uint8_t* _block
)
{
    uint32_t w[80];

    // the schedule: the block's words, then each from four before it
    for (unsigned at = 0u; at < 80u; ++at)
    {
        w[at] = (at < 16u)
                ? d_internal_big(&_block[at * 4u])
                : d_internal_rotl(w[at - 3u] ^ w[at - 8u]
                                  ^ w[at - 14u] ^ w[at - 16u],
                                  1u);
    }

    uint32_t a = _state[0];
    uint32_t b = _state[1];
    uint32_t c = _state[2];
    uint32_t d = _state[3];
    uint32_t e = _state[4];

    // the rounds
    for (unsigned at = 0u; at < 80u; ++at)
    {
        const uint32_t next = d_internal_rotl(a, 5u)
                              + d_internal_sha1_round(at, b, c, d)
                              + e
                              + w[at];

        e = d;
        d = c;
        c = d_internal_rotl(b, 30u);
        b = a;
        a = next;
    }

    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;
    _state[4] += e;

    return;
}


/*
d_internal_sha256_block
  One block compressed into SHA-256's state: the schedule of 64 words,
then 64 rounds.
*/
static void
d_internal_sha256_block(
    uint32_t*      _state,
    const uint8_t* _block
)
{
    uint32_t w[64];
    uint32_t v[8];

    // the schedule: the block's words, then each from those before it
    for (unsigned at = 0u; at < 64u; ++at)
    {
        // a word of the block, or one made from the schedule so far
        if (at < 16u)
        {
            w[at] = d_internal_big(&_block[at * 4u]);

            continue;
        }

        const uint32_t s0 = d_internal_rotr(w[at - 15u], 7u)
                            ^ d_internal_rotr(w[at - 15u], 18u)
                            ^ (w[at - 15u] >> 3);
        const uint32_t s1 = d_internal_rotr(w[at - 2u], 17u)
                            ^ d_internal_rotr(w[at - 2u], 19u)
                            ^ (w[at - 2u] >> 10);

        w[at] = w[at - 16u] + s0 + w[at - 7u] + s1;
    }

    memcpy(v, _state, sizeof(v));

    // the rounds
    for (unsigned at = 0u; at < 64u; ++at)
    {
        const uint32_t s1 = d_internal_rotr(v[4], 6u)
                            ^ d_internal_rotr(v[4], 11u)
                            ^ d_internal_rotr(v[4], 25u);
        const uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
        const uint32_t t1 = v[7] + s1 + ch + d_internal_sha256_k[at] + w[at];
        const uint32_t s0 = d_internal_rotr(v[0], 2u)
                            ^ d_internal_rotr(v[0], 13u)
                            ^ d_internal_rotr(v[0], 22u);
        const uint32_t mj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);

        memmove(&v[1], &v[0], 7u * sizeof(v[0]));
        v[4] += t1;
        v[0]  = t1 + s0 + mj;
    }

    // the rounds' words added into the state
    for (unsigned at = 0u; at < 8u; ++at)
    {
        _state[at] += v[at];
    }

    return;
}


/*
d_internal_compress
  A full block compressed, by the hash's kind.
*/
static void
d_internal_compress(
    struct d_git_hash* _hash,
    const uint8_t*     _block
)
{
    // each kind its own compression
    if (_hash->kind == D_GIT_HASH_SHA256)
    {
        d_internal_sha256_block(_hash->state, _block);
    }
    else
    {
        d_internal_sha1_block(_hash->state, _block);
    }

    return;
}


size_t
d_git_hash_size(
    enum d_git_hash_kind _kind
)
{
    return (_kind == D_GIT_HASH_SHA256) ? 32u : 20u;
}


void
d_git_hash_begin(
    struct d_git_hash*   _hash,
    enum d_git_hash_kind _kind
)
{
    static const uint32_t sha1[5]   = { 0x67452301u, 0xefcdab89u, 0x98badcfeu,
                                        0x10325476u, 0xc3d2e1f0u };
    static const uint32_t sha256[8] = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u,
                                        0xa54ff53au, 0x510e527fu, 0x9b05688cu,
                                        0x1f83d9abu, 0x5be0cd19u };

    memset(_hash, 0, sizeof(*_hash));
    _hash->kind = _kind;

    // the kind's first state
    if (_kind == D_GIT_HASH_SHA256)
    {
        memcpy(_hash->state, sha256, sizeof(sha256));
    }
    else
    {
        memcpy(_hash->state, sha1, sizeof(sha1));
    }

    return;
}


void
d_git_hash_feed(
    struct d_git_hash* _hash,
    const void*        _bytes,
    size_t             _length
)
{
    const uint8_t* bytes = _bytes;

    _hash->fed += _length;

    // the bytes, a block's worth at a time
    while (_length > 0u)
    {
        const size_t room = 64u - _hash->waiting;
        const size_t take = (_length < room) ? _length : room;

        memcpy(&_hash->block[_hash->waiting], bytes, take);
        _hash->waiting += take;
        bytes          += take;
        _length        -= take;

        // a full block is compressed
        if (_hash->waiting == 64u)
        {
            d_internal_compress(_hash, _hash->block);
            _hash->waiting = 0u;
        }
    }

    return;
}


void
d_git_hash_end(
    struct d_git_hash* _hash,
    uint8_t*           _out
)
{
    const uint64_t bits = _hash->fed * 8u;
    uint8_t        pad  = 0x80u;
    uint8_t        length[8];

    // the length, big-endian
    for (unsigned at = 0u; at < 8u; ++at)
    {
        length[at] = (uint8_t)(bits >> (56u - (at * 8u)));
    }

    d_git_hash_feed(_hash, &pad, 1u);
    pad = 0u;

    // zeros until the length fits at the block's end
    while (_hash->waiting != 56u)
    {
        d_git_hash_feed(_hash, &pad, 1u);
    }

    d_git_hash_feed(_hash, length, 8u);

    const size_t words = d_git_hash_size(_hash->kind) / 4u;

    // the state, big-endian
    for (size_t at = 0u; at < words; ++at)
    {
        _out[(at * 4u)]      = (uint8_t)(_hash->state[at] >> 24);
        _out[(at * 4u) + 1u] = (uint8_t)(_hash->state[at] >> 16);
        _out[(at * 4u) + 2u] = (uint8_t)(_hash->state[at] >> 8);
        _out[(at * 4u) + 3u] = (uint8_t)_hash->state[at];
    }

    d_git_hash_begin(_hash, _hash->kind);

    return;
}
