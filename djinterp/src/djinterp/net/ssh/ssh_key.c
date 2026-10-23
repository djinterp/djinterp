/*******************************************************************************
* djinterp [net]                                                       ssh_key.c
*
*   Definitions for ssh_key.h: key types, SHA256 fingerprints, and the
* secure random source that salts hashed known_hosts entries.
*   The digests come from net's codec rather than from an engine's crypto
* library, so fingerprints and known_hosts behave the same whichever engine
* -- or none -- is compiled.
*
*
* path:      /src/djinterp/net/ssh/ssh_key.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_key.h"  // corresponding header
// std
#include <errno.h>    // errno, EINTR
#include <limits.h>   // ULONG_MAX
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // FILE, fopen, fread, fclose
#include <stdlib.h>   // arc4random_buf where it exists
#include <string.h>   // memchr, memcpy
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"              // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"      // d_pack_bytes
#include "../../../../inc/djinterp/env/net/ssh/env_ssh.h"     // D_ENV_SSH_RANDOM_*
#include "../../../../inc/djinterp/net/codec/codec_base64.h"  // d_net_base64_encode
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // d_net_digest_compute
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"        // d_ssh_reader
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"    // d_ssh_internal_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t

#if (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_GETRANDOM)
    #include <sys/random.h>  // getrandom
#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_BCRYPT)
    #include <windows.h>     // NTSTATUS, ULONG, PUCHAR
    #include <bcrypt.h>      // BCryptGenRandom
#endif


/*
d_ssh_key_type
  Only the name is examined. The fields after it are the key's own business:
keys are compared as whole blobs, never by their parts.
*/
enum d_ssh_error
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
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    struct d_ssh_reader  reader = { 0 };
    const unsigned char* name   = NULL;
    size_t               length = 0;

    d_ssh_reader_init(&reader,
                      _key,
                      _size);

    // a single printable name of bounded length, not arbitrary bytes
    if ( (!d_ssh_read_string(&reader,
                             &name,
                             &length))             ||
         (length == 0)                             ||
         (length >= D_SSH_KEY_TYPE_SIZE)           ||
         (!d_ssh_internal_name_list_valid((const char*)name,
                                          length)) ||
         (memchr(name,
                 ',',
                 length) != NULL) )
    {
        return D_SSH_ERROR_MALFORMED;
    }

    if (length >= _capacity)
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    memcpy(_type,
           name,
           length);
    _type[length] = '\0';

    return D_SSH_OK;
}

/*
d_ssh_fingerprint
  OpenSSH's default form since 6.8: the digest's Base64 without padding.
*/
enum d_ssh_error
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
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const struct d_pack_bytes key = { _key,
                                      _size };
    uint8_t                   digest[D_NET_DIGEST_SHA256_SIZE] = { 0 };

    // the digest refuses only what the validation above already has
    if (!d_net_digest_compute(D_NET_DIGEST_SHA256,
                              key,
                              digest))
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const struct d_pack_bytes encoded = { digest,
                                          sizeof(digest) };

    memcpy(_text,
           "SHA256:",
           7);

    // the digest's base64 follows the prefix, unpadded
    const enum d_net_codec_error status =
        d_net_base64_encode(encoded,
                            D_NET_BASE64_UNPADDED,
                            _text + 7,
                            _capacity - 7,
                            NULL);

    return (status == D_NET_CODEC_OK) ? D_SSH_OK
                                      : D_SSH_ERROR_INVALID_ARGUMENT;
}

#if (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_GETRANDOM)

/*
d_ssh_internal_random_fill
  File-local: getrandom, retried across signals and short reads. Flag 0 draws
from the kernel's CSPRNG and blocks only until it is first seeded.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    size_t done = 0;

    // a large request may be satisfied in pieces
    while (done < _size)
    {
        const ssize_t got = getrandom(_buffer + done,
                                      _size - done,
                                      0);

        if (got < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return D_SSH_ERROR_TRANSPORT;
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
D_STATIC enum d_ssh_error
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    arc4random_buf(_buffer,
                   _size);

    return D_SSH_OK;
}

#elif (D_ENV_SSH_RANDOM_SOURCE == D_ENV_SSH_RANDOM_BCRYPT)

/*
d_ssh_internal_random_fill
  File-local: CNG's system-preferred RNG, in pieces no longer than a ULONG
can count.
*/
D_STATIC enum d_ssh_error
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
            return D_SSH_ERROR_TRANSPORT;
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
D_STATIC enum d_ssh_error
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    FILE* const file = fopen("/dev/urandom",
                             "rb");

    if (!file)
    {
        return D_SSH_ERROR_TRANSPORT;
    }

    const size_t got    = fread(_buffer,
                                1,
                                _size,
                                file);
    const int    closed = fclose(file);

    return ( (got == _size) &&
             (closed == 0) ) ? D_SSH_OK : D_SSH_ERROR_TRANSPORT;
}

#else

/*
d_ssh_internal_random_fill
  File-local: no secure source is known here, and nothing weaker will do.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_random_fill(
    unsigned char* _buffer,
    size_t         _size
)
{
    (void)_buffer;
    (void)_size;

    return D_SSH_ERROR_UNSUPPORTED;
}

#endif

/*
d_ssh_random
  An empty request succeeds without consulting the source.
*/
enum d_ssh_error
d_ssh_random(
    void*  _buffer,
    size_t _size
)
{
    if ( (!_buffer) &&
         (_size > 0) )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    if (_size == 0)
    {
        return D_SSH_OK;
    }

    return d_ssh_internal_random_fill((unsigned char*)_buffer,
                                      _size);
}
