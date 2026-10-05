/*******************************************************************************
* djinterp [net]                                                       ssh_key.c
*
*   Definitions for ssh_key.h, and the SHA-1, SHA-256, and HMAC-SHA1
* primitives that fingerprints and hashed known_hosts entries need.
*   The hashes are implemented here rather than borrowed from an
* engine's crypto library, so that known_hosts behaves the same whichever
* engine -- or none -- is compiled. They digest only public data (key
* blobs, salts, and host names), so they need no side-channel hardening.
*
*
* path:      /src/djinterp/net/ssh/ssh_key.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_key.h"  // corresponding header
// std
#include <errno.h>    // errno, EINTR
#include <limits.h>   // ULONG_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t, uint64_t
#include <stdio.h>    // FILE, fopen, fread, fclose
#include <stdlib.h>   // arc4random_buf where it exists
#include <string.h>   // memchr, memcpy, memmove, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/env/net/ssh/env_ssh.h"  // D_ENV_SSH_RANDOM_*
#include "../../../../inc/djinterp/net/ssh/ssh_base64.h"  // d_ssh_base64_encode
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // d_ssh_reader
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*

#if (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_GETRANDOM)
    #include <sys/random.h>  // getrandom
#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_BCRYPT)
    #include <windows.h>     // NTSTATUS, ULONG, PUCHAR
    #include <bcrypt.h>      // BCryptGenRandom
#endif


// d_ssh_internal_hash
//   a SHA-1 or SHA-256 computation. The two share 64-byte blocks, padding,
// and a big-endian bit-length field; they differ in the compression function
// and in how many state words form the digest.
struct d_ssh_internal_hash
{
    void          (*compress)(uint32_t*            _state,
                              const unsigned char* _block);
    uint32_t      state[8];   // chaining value
    uint64_t      length;     // bytes hashed so far
    unsigned char block[64];  // the partial block
    size_t        used;       // bytes in `block`
    size_t        words;      // state words in the digest
};

// d_ssh_internal_sha1_initial
//   SHA-1's initial hash value (FIPS 180-4 section 5.3.1).
D_STATIC const uint32_t d_ssh_internal_sha1_initial[5] =
{
    0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u
};

// d_ssh_internal_sha1_k
//   SHA-1's constant for each of its four stages (FIPS 180-4 section 4.2.1).
D_STATIC const uint32_t d_ssh_internal_sha1_k[4] =
{
    0x5a827999u, 0x6ed9eba1u, 0x8f1bbcdcu, 0xca62c1d6u
};

// d_ssh_internal_sha256_initial
//   SHA-256's initial hash value (FIPS 180-4 section 5.3.3).
D_STATIC const uint32_t d_ssh_internal_sha256_initial[8] =
{
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

// d_ssh_internal_sha256_k
//   SHA-256's round constants (FIPS 180-4 section 4.2.2).
D_STATIC const uint32_t d_ssh_internal_sha256_k[64] =
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
d_ssh_internal_rotl
  File-local: rotates left by 1 to 31 bits.
*/
D_STATIC uint32_t
d_ssh_internal_rotl(
    uint32_t     _value,
    unsigned int _count
)
{
    return (_value << _count) | (_value >> (32u - _count));
}

/*
d_ssh_internal_rotr
  File-local: rotates right by 1 to 31 bits.
*/
D_STATIC uint32_t
d_ssh_internal_rotr(
    uint32_t     _value,
    unsigned int _count
)
{
    return (_value >> _count) | (_value << (32u - _count));
}

/*
d_ssh_internal_sha1_f
  File-local: SHA-1's round function for a stage: choose, parity, majority,
parity.
*/
D_STATIC uint32_t
d_ssh_internal_sha1_f(
    size_t   _stage,
    uint32_t _b,
    uint32_t _c,
    uint32_t _d
)
{
    if (_stage == 0)
    {
        return (_b & _c) | (~_b & _d);
    }

    if (_stage == 2)
    {
        return (_b & _c) | (_b & _d) | (_c & _d);
    }

    return _b ^ _c ^ _d;
}

/*
d_ssh_internal_sha1_compress
  File-local: SHA-1's compression function (FIPS 180-4 section 6.1.2). The
working variables a..e live in v[0]..v[4], so each round's rotation of them
is one memmove.
*/
D_STATIC void
d_ssh_internal_sha1_compress(
    uint32_t*            _state,
    const unsigned char* _block
)
{
    uint32_t w[80];

    // the message schedule: 16 words from the block, 64 derived
    for (size_t i = 0; i < 16; i++)
    {
        w[i] = d_ssh_internal_load32(_block + (i * 4));
    }

    for (size_t i = 16; i < 80; i++)
    {
        w[i] = d_ssh_internal_rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16],
                                   1);
    }

    uint32_t v[5];

    memcpy(v, _state, sizeof(v));

    // 80 rounds in four stages of 20
    for (size_t i = 0; i < 80; i++)
    {
        const size_t   stage = i / 20;
        const uint32_t temp  = d_ssh_internal_rotl(v[0], 5)                  +
                               d_ssh_internal_sha1_f(stage, v[1], v[2], v[3]) +
                               v[4] + d_ssh_internal_sha1_k[stage] + w[i];

        memmove(v + 1, v, 4 * sizeof(uint32_t));
        v[2] = d_ssh_internal_rotl(v[2], 30);
        v[0] = temp;
    }

    for (size_t i = 0; i < 5; i++)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_ssh_internal_sha256_compress
  File-local: SHA-256's compression function (FIPS 180-4 section 6.2.2). The
working variables a..h live in v[0]..v[7]; after the memmove, v[4] holds the
old d, which becomes the new e once t1 is added.
*/
D_STATIC void
d_ssh_internal_sha256_compress(
    uint32_t*            _state,
    const unsigned char* _block
)
{
    uint32_t w[64];

    // the message schedule: 16 words from the block, 48 derived
    for (size_t i = 0; i < 16; i++)
    {
        w[i] = d_ssh_internal_load32(_block + (i * 4));
    }

    for (size_t i = 16; i < 64; i++)
    {
        const uint32_t s0 = d_ssh_internal_rotr(w[i - 15], 7)  ^
                            d_ssh_internal_rotr(w[i - 15], 18) ^
                            (w[i - 15] >> 3);
        const uint32_t s1 = d_ssh_internal_rotr(w[i - 2], 17) ^
                            d_ssh_internal_rotr(w[i - 2], 19) ^
                            (w[i - 2] >> 10);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t v[8];

    memcpy(v, _state, sizeof(v));

    // 64 rounds
    for (size_t i = 0; i < 64; i++)
    {
        const uint32_t s1     = d_ssh_internal_rotr(v[4], 6)  ^
                                d_ssh_internal_rotr(v[4], 11) ^
                                d_ssh_internal_rotr(v[4], 25);
        const uint32_t choice = (v[4] & v[5]) ^ (~v[4] & v[6]);
        const uint32_t t1     = v[7] + s1 + choice +
                                d_ssh_internal_sha256_k[i] + w[i];
        const uint32_t s0     = d_ssh_internal_rotr(v[0], 2)  ^
                                d_ssh_internal_rotr(v[0], 13) ^
                                d_ssh_internal_rotr(v[0], 22);
        const uint32_t major  = (v[0] & v[1]) ^
                                (v[0] & v[2]) ^
                                (v[1] & v[2]);

        memmove(v + 1, v, 7 * sizeof(uint32_t));
        v[4] += t1;
        v[0]  = t1 + s0 + major;
    }

    for (size_t i = 0; i < 8; i++)
    {
        _state[i] += v[i];
    }

    return;
}

/*
d_ssh_internal_hash_init
  File-local: starts a SHA-256 computation, or a SHA-1 one.
*/
D_STATIC void
d_ssh_internal_hash_init(
    struct d_ssh_internal_hash* _hash,
    bool                        _sha256
)
{
    memset(_hash, 0, sizeof(*_hash));

    if (_sha256)
    {
        memcpy(_hash->state,
               d_ssh_internal_sha256_initial,
               sizeof(d_ssh_internal_sha256_initial));
        _hash->compress = d_ssh_internal_sha256_compress;
        _hash->words    = 8;
    }
    else
    {
        memcpy(_hash->state,
               d_ssh_internal_sha1_initial,
               sizeof(d_ssh_internal_sha1_initial));
        _hash->compress = d_ssh_internal_sha1_compress;
        _hash->words    = 5;
    }

    return;
}

/*
d_ssh_internal_hash_update
  File-local: feeds bytes through the partial block, compressing each time it
fills.
*/
D_STATIC void
d_ssh_internal_hash_update(
    struct d_ssh_internal_hash* _hash,
    const void*                 _data,
    size_t                      _size
)
{
    const unsigned char* bytes     = (const unsigned char*)_data;
    size_t               remaining = _size;

    _hash->length += (uint64_t)_size;

    // top up the block, compressing whenever it completes
    while (remaining > 0)
    {
        size_t take = sizeof(_hash->block) - _hash->used;

        if (take > remaining)
        {
            take = remaining;
        }

        memcpy(_hash->block + _hash->used, bytes, take);
        _hash->used += take;
        bytes       += take;
        remaining   -= take;

        if (_hash->used == sizeof(_hash->block))
        {
            _hash->compress(_hash->state, _hash->block);
            _hash->used = 0;
        }
    }

    return;
}

/*
d_ssh_internal_hash_final
  File-local: pads with a 1 bit, zeros to 56 bytes into a block, and the
message length in bits, then writes the digest big-endian.
*/
D_STATIC void
d_ssh_internal_hash_final(
    struct d_ssh_internal_hash* _hash,
    unsigned char*              _digest
)
{
    const uint64_t      bits   = _hash->length * 8u;
    const unsigned char marker = 0x80;
    const unsigned char zero   = 0;

    d_ssh_internal_hash_update(_hash, &marker, 1);

    // zeros until exactly eight bytes of the block remain
    while (_hash->used != (sizeof(_hash->block) - 8))
    {
        d_ssh_internal_hash_update(_hash, &zero, 1);
    }

    unsigned char tail[8];

    d_ssh_internal_store64(tail, bits);
    d_ssh_internal_hash_update(_hash, tail, sizeof(tail));

    for (size_t i = 0; i < _hash->words; i++)
    {
        d_ssh_internal_store32(_digest + (i * 4), _hash->state[i]);
    }

    return;
}

/*
d_ssh_internal_sha256
  Internal: the SHA-256 digest of a buffer.
*/
void
d_ssh_internal_sha256(
    const void*    _data,
    size_t         _size,
    unsigned char* _digest
)
{
    struct d_ssh_internal_hash hash;

    d_ssh_internal_hash_init(&hash, true);
    d_ssh_internal_hash_update(&hash, _data, _size);
    d_ssh_internal_hash_final(&hash, _digest);

    return;
}

/*
d_ssh_internal_hmac_sha1
  Internal: HMAC-SHA1 (RFC 2104), which hashed known_hosts entries compute
with their 20-byte salt as the key.
*/
void
d_ssh_internal_hmac_sha1(
    const unsigned char* _key,
    size_t               _key_size,
    const void*          _data,
    size_t               _size,
    unsigned char*       _mac
)
{
    unsigned char              key[64] = { 0 };
    struct d_ssh_internal_hash hash;

    // a key longer than a block is replaced by its digest
    if (_key_size > sizeof(key))
    {
        d_ssh_internal_hash_init(&hash, false);
        d_ssh_internal_hash_update(&hash, _key, _key_size);
        d_ssh_internal_hash_final(&hash, key);
    }
    else if (_key_size > 0)
    {
        memcpy(key, _key, _key_size);
    }

    unsigned char pad[64];
    unsigned char inner[D_SSH_INTERNAL_SHA1_SIZE];

    // the inner digest covers key ^ ipad, then the data
    for (size_t i = 0; i < sizeof(pad); i++)
    {
        pad[i] = (unsigned char)(key[i] ^ 0x36u);
    }

    d_ssh_internal_hash_init(&hash, false);
    d_ssh_internal_hash_update(&hash, pad, sizeof(pad));
    d_ssh_internal_hash_update(&hash, _data, _size);
    d_ssh_internal_hash_final(&hash, inner);

    // the outer digest covers key ^ opad, then the inner digest
    for (size_t i = 0; i < sizeof(pad); i++)
    {
        pad[i] = (unsigned char)(key[i] ^ 0x5Cu);
    }

    d_ssh_internal_hash_init(&hash, false);
    d_ssh_internal_hash_update(&hash, pad, sizeof(pad));
    d_ssh_internal_hash_update(&hash, inner, sizeof(inner));
    d_ssh_internal_hash_final(&hash, _mac);

    return;
}

/*
d_ssh_key_type
  Only the name is examined. The fields after it are the key's own business:
keys are compared as whole blobs, never by their parts.
*/
enum d_ssh_status
d_ssh_key_type(
    const unsigned char* _key,
    size_t               _size,
    char*                _type,
    size_t               _capacity
)
{
    if ( (_type) &&
         (_capacity > 0) )
    {
        _type[0] = '\0';
    }

    if ( (!_key)  ||
         (!_type) ||
         (_capacity == 0) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    struct d_ssh_reader  reader;
    const unsigned char* name   = NULL;
    size_t               length = 0;

    d_ssh_reader_init(&reader, _key, _size);

    // a single printable name of bounded length, not arbitrary bytes
    if ( (!d_ssh_read_string(&reader, &name, &length))                     ||
         (length == 0)                                                     ||
         (length >= D_SSH_KEY_TYPE_SIZE)                                   ||
         (!d_ssh_internal_name_list_valid((const char*)name, length))      ||
         (memchr(name, ',', length) != NULL) )
    {
        return D_SSH_ERR_FORMAT;
    }

    if (length >= _capacity)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    memcpy(_type, name, length);
    _type[length] = '\0';

    return D_SSH_OK;
}

/*
d_ssh_fingerprint
  OpenSSH's default form since 6.8: the digest's Base64 without padding.
*/
enum d_ssh_status
d_ssh_fingerprint(
    const unsigned char* _key,
    size_t               _size,
    char*                _text,
    size_t               _capacity
)
{
    if ( (_text) &&
         (_capacity > 0) )
    {
        _text[0] = '\0';
    }

    if ( (!_key)        ||
         (!_text)       ||
         (_size == 0)   ||
         (_capacity < D_SSH_FINGERPRINT_SIZE) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    unsigned char digest[D_SSH_INTERNAL_SHA256_SIZE];

    d_ssh_internal_sha256(_key, _size, digest);
    memcpy(_text, "SHA256:", 7);

    return d_ssh_base64_encode(digest,
                               sizeof(digest),
                               false,
                               _text + 7,
                               _capacity - 7);
}

#if (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_GETRANDOM)

/*
d_ssh_internal_random_fill
  File-local: getrandom, retried across signals and short reads. Flag 0 draws
from the kernel's CSPRNG and blocks only until it is first seeded.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    size_t done = 0;

    // a large request may be satisfied in pieces
    while (done < _size)
    {
        const ssize_t got = getrandom(_buffer + done, _size - done, 0);

        if (got < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return D_SSH_ERR_IO;
        }

        done += (size_t)got;
    }

    return D_SSH_OK;
}

#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_ARC4RANDOM)

/*
d_ssh_internal_random_fill
  File-local: arc4random_buf, which cannot fail on the platforms that select
it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    arc4random_buf(_buffer, _size);

    return D_SSH_OK;
}

#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_BCRYPT)

/*
d_ssh_internal_random_fill
  File-local: CNG's system-preferred RNG, in pieces no longer than a ULONG
can count.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    size_t done = 0;

    // BCryptGenRandom takes a ULONG length
    while (done < _size)
    {
        const size_t   remaining = _size - done;
        const ULONG    chunk     = (remaining > (size_t)ULONG_MAX)
                                       ? ULONG_MAX
                                       : (ULONG)remaining;
        const NTSTATUS result    =
            BCryptGenRandom(NULL,
                            (PUCHAR)(_buffer + done),
                            chunk,
                            BCRYPT_USE_SYSTEM_PREFERRED_RNG);

        if (result < 0)
        {
            return D_SSH_ERR_IO;
        }

        done += (size_t)chunk;
    }

    return D_SSH_OK;
}

#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_DEV_URANDOM)

/*
d_ssh_internal_random_fill
  File-local: reads /dev/urandom, the last resort on POSIX-like systems with
no dedicated call.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    FILE* const file = fopen("/dev/urandom", "rb");

    if (!file)
    {
        return D_SSH_ERR_IO;
    }

    const size_t got    = fread(_buffer, 1, _size, file);
    const int    closed = fclose(file);

    return ( (got == _size) &&
             (closed == 0) ) ? D_SSH_OK : D_SSH_ERR_IO;
}

#else

/*
d_ssh_internal_random_fill
  File-local: no secure source is known here, and nothing weaker will do.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    (void)_buffer;
    (void)_size;

    return D_SSH_ERR_UNSUPPORTED;
}

#endif

/*
d_ssh_random
  An empty request succeeds without consulting the source.
*/
enum d_ssh_status
d_ssh_random(
    void*  _buffer,
    size_t _size
)
{
    if ( (!_buffer) &&
         (_size > 0) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    if (_size == 0)
    {
        return D_SSH_OK;
    }

    return d_ssh_internal_random_fill((unsigned char*)_buffer, _size);
}
