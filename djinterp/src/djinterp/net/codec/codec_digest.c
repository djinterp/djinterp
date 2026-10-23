/*******************************************************************************
* djinterp [net]                                                  codec_digest.c
*
* Definitions for codec_digest.h: MD5, SHA-1 and SHA-256, HMAC over each, and
* the comparison and wiping that digests of secrets need.
*   The three functions share a 64-octet block, the buffering that fills it,
* and the shape of their padding. They differ in their compression functions,
* in the byte order of their words and of the length that ends the padding,
* and in how many words of state make the digest; the state's kind selects
* each of these, so one state type and one buffer serve all three.
*   No step branches on, or indexes a table by, the data: what varies with
* the input is how many blocks there are, so running time depends on lengths
* alone.
*
*
* path:      /src/djinterp/net/codec/codec_digest.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcpy, memmove, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_bytes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t, uint64_t


// D_INTERNAL_NET_DIGEST_LENGTH_AT
//   constant: where in the last block the message length begins: the eight
// octets that end every padded message.
#define D_INTERNAL_NET_DIGEST_LENGTH_AT (D_NET_DIGEST_BLOCK_SIZE - 8)

// d_internal_net_md5_initial
//   MD5's initial chaining value, A to D (RFC 1321 section 3.3).
D_STATIC const uint32_t d_internal_net_md5_initial[4] =
{
    0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u
};

// d_internal_net_md5_k
//   MD5's additive constants, floor(|sin(i + 1)| * 2^32) (RFC 1321 section
// 3.4).
D_STATIC const uint32_t d_internal_net_md5_k[64] =
{
    0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
    0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
    0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
    0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
    0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
    0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
    0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
    0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
    0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
    0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
    0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
    0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
    0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
    0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
    0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
    0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
};

// d_internal_net_md5_shift
//   MD5's left rotations: each round repeats its four (RFC 1321 section 3.4).
D_STATIC const unsigned char d_internal_net_md5_shift[4][4] =
{
    { 7, 12, 17, 22 },
    { 5,  9, 14, 20 },
    { 4, 11, 16, 23 },
    { 6, 10, 15, 21 }
};

// d_internal_net_md5_order
//   how each MD5 round orders the message words: step i of a round takes word
// (multiplier * i + offset) mod 16 (RFC 1321 section 3.4).
D_STATIC const unsigned char d_internal_net_md5_order[4][2] =
{
    { 1, 0 },
    { 5, 1 },
    { 3, 5 },
    { 7, 0 }
};

// d_internal_net_sha1_initial
//   SHA-1's initial hash value (FIPS 180-4 section 5.3.1).
D_STATIC const uint32_t d_internal_net_sha1_initial[5] =
{
    0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u
};

// d_internal_net_sha1_k
//   SHA-1's constant for each of its four stages (FIPS 180-4 section 4.2.1).
D_STATIC const uint32_t d_internal_net_sha1_k[4] =
{
    0x5a827999u, 0x6ed9eba1u, 0x8f1bbcdcu, 0xca62c1d6u
};

// d_internal_net_sha256_initial
//   SHA-256's initial hash value (FIPS 180-4 section 5.3.3).
D_STATIC const uint32_t d_internal_net_sha256_initial[8] =
{
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

// d_internal_net_sha256_k
//   SHA-256's round constants (FIPS 180-4 section 4.2.2).
D_STATIC const uint32_t d_internal_net_sha256_k[64] =
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
d_internal_net_load32_be
  A word from four octets, most significant first, as SHA reads its blocks.
Assembled by shifts, so the host's own byte order never matters.
*/
D_STATIC uint32_t
d_internal_net_load32_be(
    const uint8_t* _bytes
)
{
    return ( ((uint32_t)_bytes[0] << 24) |
             ((uint32_t)_bytes[1] << 16) |
             ((uint32_t)_bytes[2] << 8)  |
             (uint32_t)_bytes[3] );
}

/*
d_internal_net_load32_le
  A word from four octets, least significant first, as MD5 reads its blocks.
*/
D_STATIC uint32_t
d_internal_net_load32_le(
    const uint8_t* _bytes
)
{
    return ( (uint32_t)_bytes[0]         |
             ((uint32_t)_bytes[1] << 8)  |
             ((uint32_t)_bytes[2] << 16) |
             ((uint32_t)_bytes[3] << 24) );
}

/*
d_internal_net_store32
  Writes a word as four octets, most significant first unless `_little` asks
for the reverse: SHA's order, or MD5's.
*/
D_STATIC void
d_internal_net_store32(
    uint8_t* _bytes,
    uint32_t _value,
    bool     _little
)
{
    // octet i holds bits 8i up for little-endian, 24 - 8i up for big-endian
    for (unsigned int i = 0; i < 4u; i++)
    {
        const unsigned int shift = (_little) ? (8u * i)
                                             : (24u - (8u * i));

        _bytes[i] = (uint8_t)(_value >> shift);
    }

    return;
}

/*
d_internal_net_rotl
  Rotates a word left by 1 to 31 bits; a count of 0 or 32 would shift by the
full width, which C leaves undefined.
*/
D_STATIC uint32_t
d_internal_net_rotl(
    uint32_t     _value,
    unsigned int _count
)
{
    return (_value << _count) | (_value >> (32u - _count));
}

/*
d_internal_net_md5_mix
  MD5's mixing function for a round: F, G, H and I of RFC 1321 section 3.4.
The round is the step's, never the data's, so the branch reveals nothing.
*/
D_STATIC uint32_t
d_internal_net_md5_mix(
    size_t   _round,
    uint32_t _b,
    uint32_t _c,
    uint32_t _d
)
{
    switch (_round)
    {
        case 0:
            return (_b & _c) | (~_b & _d);

        case 1:
            return (_b & _d) | (_c & ~_d);

        case 2:
            return _b ^ _c ^ _d;

        default:
            return _c ^ (_b | ~_d);
    }
}

/*
d_internal_net_md5_compress
  One block through MD5 (RFC 1321 section 3.4), in the loop form: the four
rounds differ only in their mixing function, the order in which they take
the message words, and their rotations, which the tables supply. The working
variables a..d live in v[0]..v[3].
*/
D_STATIC void
d_internal_net_md5_compress(
    uint32_t*      _state,
    const uint8_t* _block
)
{
    uint32_t words[16] = { 0 };

    // the block as sixteen little-endian words
    for (size_t i = 0; i < 16; i++)
    {
        words[i] = d_internal_net_load32_le(_block + (i * 4));
    }

    uint32_t v[4] = { 0 };

    memcpy(v,
           _state,
           sizeof(v));

    // sixty-four steps in four rounds of sixteen
    for (size_t i = 0; i < 64; i++)
    {
        const size_t        round = i / 16;
        const unsigned char scale = d_internal_net_md5_order[round][0];
        const unsigned char start = d_internal_net_md5_order[round][1];
        const unsigned char shift = d_internal_net_md5_shift[round][i % 4];
        const uint32_t      mixed = d_internal_net_md5_mix(round,
                                                           v[1],
                                                           v[2],
                                                           v[3]);
        const uint32_t      sum   = v[0] + mixed + d_internal_net_md5_k[i] +
                                    words[((scale * i) + start) % 16];

        v[0] = v[3];
        v[3] = v[2];
        v[2] = v[1];
        v[1] = v[1] + d_internal_net_rotl(sum,
                                          shift);
    }

    // the block's result joins the chaining value
    for (size_t i = 0; i < 4; i++)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_internal_net_sha1_mix
  SHA-1's function for a stage (FIPS 180-4 section 4.1.1): choose, parity,
majority, parity. As with MD5, the stage is the step's, not the data's.
*/
D_STATIC uint32_t
d_internal_net_sha1_mix(
    size_t   _stage,
    uint32_t _b,
    uint32_t _c,
    uint32_t _d
)
{
    switch (_stage)
    {
        case 0:
            return (_b & _c) | (~_b & _d);

        case 2:
            return (_b & _c) | (_b & _d) | (_c & _d);

        default:
            return _b ^ _c ^ _d;
    }
}

/*
d_internal_net_sha1_compress
  One block through SHA-1 (FIPS 180-4 section 6.1.2). The working variables
a..e live in v[0]..v[4], so each round's rotation of them is one memmove.
*/
D_STATIC void
d_internal_net_sha1_compress(
    uint32_t*      _state,
    const uint8_t* _block
)
{
    uint32_t w[80] = { 0 };

    // the message schedule: 16 words from the block, 64 derived
    for (size_t i = 0; i < 16; i++)
    {
        w[i] = d_internal_net_load32_be(_block + (i * 4));
    }

    for (size_t i = 16; i < 80; i++)
    {
        w[i] = d_internal_net_rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16],
                                   1);
    }

    uint32_t v[5] = { 0 };

    memcpy(v,
           _state,
           sizeof(v));

    // 80 rounds in four stages of 20
    for (size_t i = 0; i < 80; i++)
    {
        const size_t   stage   = i / 20;
        const uint32_t rotated = d_internal_net_rotl(v[0],
                                                     5);
        const uint32_t mixed   = d_internal_net_sha1_mix(stage,
                                                         v[1],
                                                         v[2],
                                                         v[3]);
        const uint32_t temp    = rotated + mixed + v[4] +
                                 d_internal_net_sha1_k[stage] + w[i];

        memmove(v + 1,
                v,
                4 * sizeof(uint32_t));
        v[2] = d_internal_net_rotl(v[2],
                                   30);
        v[0] = temp;
    }

    // the block's result joins the chaining value
    for (size_t i = 0; i < 5; i++)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_internal_net_sha256_big_sigma0
  The function FIPS 180-4 section 4.1.2 writes as a capital sigma with
subscript 0, on the working variable a: the word rotated right by 2, by 13
and by 22, combined. Each rotation is its pair of shifts.
*/
D_STATIC uint32_t
d_internal_net_sha256_big_sigma0(
    uint32_t _value
)
{
    return ( ((_value >> 2)  | (_value << 30)) ^
             ((_value >> 13) | (_value << 19)) ^
             ((_value >> 22) | (_value << 10)) );
}

/*
d_internal_net_sha256_big_sigma1
  The capital sigma with subscript 1, on the working variable e: the word
rotated right by 6, by 11 and by 25, combined.
*/
D_STATIC uint32_t
d_internal_net_sha256_big_sigma1(
    uint32_t _value
)
{
    return ( ((_value >> 6)  | (_value << 26)) ^
             ((_value >> 11) | (_value << 21)) ^
             ((_value >> 25) | (_value << 7)) );
}

/*
d_internal_net_sha256_small_sigma0
  The small sigma with subscript 0, on the message schedule: the word rotated
right by 7 and by 18, and shifted right by 3, combined.
*/
D_STATIC uint32_t
d_internal_net_sha256_small_sigma0(
    uint32_t _value
)
{
    return ( ((_value >> 7)  | (_value << 25)) ^
             ((_value >> 18) | (_value << 14)) ^
             (_value >> 3) );
}

/*
d_internal_net_sha256_small_sigma1
  The small sigma with subscript 1, on the message schedule: the word rotated
right by 17 and by 19, and shifted right by 10, combined.
*/
D_STATIC uint32_t
d_internal_net_sha256_small_sigma1(
    uint32_t _value
)
{
    return ( ((_value >> 17) | (_value << 15)) ^
             ((_value >> 19) | (_value << 13)) ^
             (_value >> 10) );
}

/*
d_internal_net_sha256_compress
  One block through SHA-256 (FIPS 180-4 section 6.2.2). The working variables
a..h live in v[0]..v[7]; after the memmove, v[4] holds the old d, which
becomes the new e once t1 is added.
*/
D_STATIC void
d_internal_net_sha256_compress(
    uint32_t*      _state,
    const uint8_t* _block
)
{
    uint32_t w[64] = { 0 };

    // the message schedule: 16 words from the block, 48 derived
    for (size_t i = 0; i < 16; i++)
    {
        w[i] = d_internal_net_load32_be(_block + (i * 4));
    }

    for (size_t i = 16; i < 64; i++)
    {
        const uint32_t s0 = d_internal_net_sha256_small_sigma0(w[i - 15]);
        const uint32_t s1 = d_internal_net_sha256_small_sigma1(w[i - 2]);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t v[8] = { 0 };

    memcpy(v,
           _state,
           sizeof(v));

    // 64 rounds
    for (size_t i = 0; i < 64; i++)
    {
        const uint32_t s1     = d_internal_net_sha256_big_sigma1(v[4]);
        const uint32_t choice = (v[4] & v[5]) ^ (~v[4] & v[6]);
        const uint32_t t1     = v[7] + s1 + choice +
                                d_internal_net_sha256_k[i] + w[i];
        const uint32_t s0     = d_internal_net_sha256_big_sigma0(v[0]);
        const uint32_t major  = (v[0] & v[1]) ^
                                (v[0] & v[2]) ^
                                (v[1] & v[2]);

        memmove(v + 1,
                v,
                7 * sizeof(uint32_t));
        v[4] += t1;
        v[0]  = t1 + s0 + major;
    }

    // the block's result joins the chaining value
    for (size_t i = 0; i < 8; i++)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_internal_net_digest_compress
  The full block through the compression function the state's kind names.
Only usable states reach here, so every kind that arrives has a case.
*/
D_STATIC void
d_internal_net_digest_compress(
    struct d_net_digest* _digest
)
{
    switch (_digest->kind)
    {
        case D_NET_DIGEST_MD5:
            d_internal_net_md5_compress(_digest->state,
                                        _digest->block);
            break;

        case D_NET_DIGEST_SHA1:
            d_internal_net_sha1_compress(_digest->state,
                                         _digest->block);
            break;

        case D_NET_DIGEST_SHA256:
            d_internal_net_sha256_compress(_digest->state,
                                           _digest->block);
            break;

        default:
            break;
    }

    return;
}

/*
d_internal_net_digest_pad
  The padding the three functions share (RFC 1321 section 3.1, FIPS 180-4
section 5.1.1): a 1 bit, zeros until eight octets of a block remain, then
the message's length in bits -- little-endian for MD5, big-endian for SHA.
When the 1 bit leaves fewer than eight octets, the zeros fill this block and
the length ends another. The length is read before the 1 bit is added, which
is not part of the message.
*/
D_STATIC void
d_internal_net_digest_pad(
    struct d_net_digest* _digest
)
{
    const uint64_t bits   = _digest->length * 8u;
    const bool     little = (_digest->kind == D_NET_DIGEST_MD5);

    _digest->block[_digest->used++] = 0x80u;

    // no room left for the length: finish this block with zeros
    if (_digest->used > D_INTERNAL_NET_DIGEST_LENGTH_AT)
    {
        memset(_digest->block + _digest->used,
               0,
               D_NET_DIGEST_BLOCK_SIZE - _digest->used);
        d_internal_net_digest_compress(_digest);
        _digest->used = 0;
    }

    memset(_digest->block + _digest->used,
           0,
           D_INTERNAL_NET_DIGEST_LENGTH_AT - _digest->used);

    // the length's eight octets, in the function's byte order
    for (unsigned int i = 0; i < 8u; i++)
    {
        const unsigned int shift = (little) ? (8u * i)
                                            : (56u - (8u * i));

        _digest->block[D_INTERNAL_NET_DIGEST_LENGTH_AT + i] =
            (uint8_t)(bits >> shift);
    }

    d_internal_net_digest_compress(_digest);

    return;
}

/*
d_net_digest_size
  A switch rather than a table indexed by the kind, so a value that names no
function cannot read outside one.
*/
size_t
d_net_digest_size(
    enum d_net_digest_kind _kind
)
{
    switch (_kind)
    {
        case D_NET_DIGEST_MD5:
            return D_NET_DIGEST_MD5_SIZE;

        case D_NET_DIGEST_SHA1:
            return D_NET_DIGEST_SHA1_SIZE;

        case D_NET_DIGEST_SHA256:
            return D_NET_DIGEST_SHA256_SIZE;

        default:
            return 0;
    }
}

/*
d_net_digest_init
  Zeroing first makes the state unusable, which is what it stays when the kind
names no function; only a kind with an initial value replaces that.
*/
void
d_net_digest_init(
    struct d_net_digest*   _digest,
    enum d_net_digest_kind _kind
)
{
    // parameter validation
    if (_digest == NULL)
    {
        return;
    }

    memset(_digest,
           0,
           sizeof(*_digest));

    // the function's initial chaining value
    switch (_kind)
    {
        case D_NET_DIGEST_MD5:
            memcpy(_digest->state,
                   d_internal_net_md5_initial,
                   sizeof(d_internal_net_md5_initial));
            break;

        case D_NET_DIGEST_SHA1:
            memcpy(_digest->state,
                   d_internal_net_sha1_initial,
                   sizeof(d_internal_net_sha1_initial));
            break;

        case D_NET_DIGEST_SHA256:
            memcpy(_digest->state,
                   d_internal_net_sha256_initial,
                   sizeof(d_internal_net_sha256_initial));
            break;

        default:
            return;
    }

    _digest->kind = _kind;

    return;
}

/*
d_net_digest_update
  Copies as much of the data as the block has room for, compresses the block
when it fills, and repeats; a run of whole blocks still passes through the
buffer, which keeps one path for every length. Refused data wipes the state,
so d_net_digest_final finds it unusable and says so.
*/
void
d_net_digest_update(
    struct d_net_digest* _digest,
    const void*          _data,
    size_t               _size
)
{
    // a missing or unusable digest is left as it is
    if ( (_digest == NULL) ||
         (d_net_digest_size(_digest->kind) == 0) )
    {
        return;
    }

    // data that is not there spoils the digest, for final to report
    if ( (_data == NULL) &&
         (_size > 0) )
    {
        d_net_wipe(_digest,
                   sizeof(*_digest));

        return;
    }

    const uint8_t* bytes     = (const uint8_t*)_data;
    size_t         remaining = _size;

    _digest->length += (uint64_t)_size;

    // top up the block, compressing whenever it fills
    while (remaining > 0)
    {
        const size_t room = D_NET_DIGEST_BLOCK_SIZE - _digest->used;
        const size_t take = (room < remaining) ? room
                                               : remaining;

        memcpy(_digest->block + _digest->used,
               bytes,
               take);
        _digest->used += take;
        bytes         += take;
        remaining     -= take;

        // a full block is compressed at once
        if (_digest->used == D_NET_DIGEST_BLOCK_SIZE)
        {
            d_internal_net_digest_compress(_digest);
            _digest->used = 0;
        }
    }

    return;
}

/*
d_net_digest_final
  Pads, then writes the chaining value as the digest: four little-endian words
for MD5, five or eight big-endian words for SHA-1 and SHA-256. The state is
wiped on every path, a refused one included, since what it holds was derived
from the data, which may be a secret.
*/
bool
d_net_digest_final(
    struct d_net_digest* _digest,
    uint8_t*             _out
)
{
    // parameter validation
    if (_digest == NULL)
    {
        return false;
    }

    const size_t size   = d_net_digest_size(_digest->kind);
    const bool   usable = ( (size > 0) &&
                            (_out != NULL) );

    // a usable digest is padded and written out
    if (usable)
    {
        const bool little = (_digest->kind == D_NET_DIGEST_MD5);

        d_internal_net_digest_pad(_digest);

        // the digest is the first size / 4 words of the chaining value
        for (size_t i = 0; i < size / 4; i++)
        {
            d_internal_net_store32(_out + (i * 4),
                                   _digest->state[i],
                                   little);
        }
    }

    d_net_wipe(_digest,
               sizeof(*_digest));

    return usable;
}

/*
d_net_digest_compute
  The incremental calls in one; d_net_digest_final reports what the others
refused.
*/
bool
d_net_digest_compute(
    enum d_net_digest_kind _kind,
    struct d_pack_bytes    _data,
    uint8_t*               _out
)
{
    struct d_net_digest digest = { 0 };

    d_net_digest_init(&digest,
                      _kind);
    d_net_digest_update(&digest,
                        _data.data,
                        _data.size);

    return d_net_digest_final(&digest,
                              _out);
}

/*
d_internal_net_hmac_pass
  One of HMAC's two digests (RFC 2104 section 2): the key block XORed with a
pad octet, then the data. The padded key is a secret, and is wiped as soon as
the digest has read it.
*/
D_STATIC bool
d_internal_net_hmac_pass(
    enum d_net_digest_kind _kind,
    const uint8_t*         _key,
    uint8_t                _pad,
    struct d_pack_bytes    _data,
    uint8_t*               _out
)
{
    uint8_t             padded[D_NET_DIGEST_BLOCK_SIZE] = { 0 };
    struct d_net_digest digest                          = { 0 };

    // the key block, XORed with the pad
    for (size_t i = 0; i < sizeof(padded); i++)
    {
        padded[i] = (uint8_t)(_key[i] ^ _pad);
    }

    d_net_digest_init(&digest,
                      _kind);
    d_net_digest_update(&digest,
                        padded,
                        sizeof(padded));
    d_net_wipe(padded,
               sizeof(padded));
    d_net_digest_update(&digest,
                        _data.data,
                        _data.size);

    return d_net_digest_final(&digest,
                              _out);
}

/*
d_net_hmac
  The key is copied into a zeroed block first -- or, longer than a block, its
digest is -- so `_out` may overlap it; the inner MAC goes to a buffer of its
own, so `_out` may overlap the data too. Both are secrets derived from the
key, and are wiped before returning.
*/
bool
d_net_hmac(
    enum d_net_digest_kind _kind,
    struct d_pack_bytes    _key,
    struct d_pack_bytes    _data,
    uint8_t*               _out
)
{
    // parameter validation
    if ( (d_net_digest_size(_kind) == 0)         ||
         (_out == NULL)                          ||
         ( (_key.data == NULL) &&
           (_key.size > 0) )                     ||
         ( (_data.data == NULL) &&
           (_data.size > 0) ) )
    {
        return false;
    }

    uint8_t key[D_NET_DIGEST_BLOCK_SIZE] = { 0 };
    uint8_t inner[D_NET_DIGEST_MAX_SIZE] = { 0 };
    bool    done                         = true;

    // a key longer than a block stands in by its digest
    if (_key.size > sizeof(key))
    {
        done = d_net_digest_compute(_kind,
                                    _key,
                                    key);
    }
    else if (_key.size > 0)
    {
        memcpy(key,
               _key.data,
               _key.size);
    }

    // the inner digest: H((K ^ ipad) || data)
    if (done)
    {
        done = d_internal_net_hmac_pass(_kind,
                                        key,
                                        0x36u,
                                        _data,
                                        inner);
    }

    const struct d_pack_bytes inner_bytes = { inner,
                                              d_net_digest_size(_kind) };

    // the outer digest, which is the MAC: H((K ^ opad) || inner)
    if (done)
    {
        done = d_internal_net_hmac_pass(_kind,
                                        key,
                                        0x5cu,
                                        inner_bytes,
                                        _out);
    }

    d_net_wipe(key,
               sizeof(key));
    d_net_wipe(inner,
               sizeof(inner));

    return done;
}

/*
d_net_digest_equal
  Every octet is compared whatever the first difference, and the differences
are gathered with OR, so the time taken does not show how many leading octets
matched.
*/
bool
d_net_digest_equal(
    const void* _a,
    const void* _b,
    size_t      _size
)
{
    // parameter validation
    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return false;
    }

    const uint8_t* a          = (const uint8_t*)_a;
    const uint8_t* b          = (const uint8_t*)_b;
    unsigned int   difference = 0;

    // gather every difference before deciding
    for (size_t i = 0; i < _size; i++)
    {
        difference |= (unsigned int)(a[i] ^ b[i]);
    }

    return (difference == 0);
}

/*
d_net_wipe
  The stores go through a volatile pointer: each is then a side effect the
compiler must keep, where a memset of memory never read again may be dropped
as dead.
*/
void
d_net_wipe(
    void*  _data,
    size_t _size
)
{
    // parameter validation
    if (_data == NULL)
    {
        return;
    }

    volatile uint8_t* const bytes = (volatile uint8_t*)_data;

    // one volatile store per octet
    for (size_t i = 0; i < _size; i++)
    {
        bytes[i] = 0;
    }

    return;
}
