/*******************************************************************************
* djinterp [c]                                                           emoji.c
*
* UTF-8 encoders for emoji.h.
*   Defines d_text_emoji_utf8_encode, which writes one code point's UTF-8 into
* the caller's buffer, and, where the compiler offers thread-local storage,
* d_text_emoji_utf8 and d_text_emoji_utf8_seq, which return one code point or
* a sequence of them as a string in a buffer that belongs to the calling
* thread. All three accept exactly the Unicode scalar values, and agree on it
* by validating through one helper.
*
*
* path:      /src/djinterp/c/text/symbol/emoji/emoji.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/text/symbol/emoji/emoji.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_THREAD_LOCAL
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


/*
d_internal_emoji_utf8_length
  The number of bytes the UTF-8 encoding of `_cp` takes, 1 to 4, or 0 when
`_cp` is not a Unicode scalar value: a surrogate, U+D800 to U+DFFF, which
UTF-8 cannot carry, or a value past U+10FFFF. The three public functions
validate through this one test, so they refuse exactly the same values.
*/
static size_t
d_internal_emoji_utf8_length(
    uint32_t _cp
)
{
    // surrogates and values past the last code point have no encoding
    if ( ( (_cp >= 0xD800u)  &&
           (_cp <= 0xDFFFu) ) ||
         (_cp > 0x10FFFFu) )
    {
        return 0;
    }

    // one byte through U+007F
    if (_cp < 0x80u)
    {
        return 1;
    }

    // two bytes through U+07FF
    if (_cp < 0x800u)
    {
        return 2;
    }

    // three bytes through U+FFFF
    if (_cp < 0x10000u)
    {
        return 3;
    }

    return 4;
}

/*
d_text_emoji_utf8_encode
  Writes through an unsigned char view of `_out`. Lead and continuation bytes
are above 0x7F, and converting such a value to a signed char is
implementation-defined, while storing an unsigned char into any object's
bytes is not. Nothing is written until the code point has been validated.
*/
size_t
d_text_emoji_utf8_encode(
    uint32_t _cp,
    char*    _out
)
{
    // a missing buffer is refused before anything is computed
    if (!_out)
    {
        return 0;
    }

    const size_t length = d_internal_emoji_utf8_length(_cp);

    // not a scalar value: nothing is written
    if (length == 0)
    {
        return 0;
    }

    unsigned char* bytes = (unsigned char*)_out;

    // the lead byte carries the length and the top bits; each continuation
    // byte carries six more
    switch (length)
    {
        case 1:
            bytes[0] = (unsigned char)_cp;
            break;

        case 2:
            bytes[0] = (unsigned char)(0xC0u | (_cp >> 6));
            bytes[1] = (unsigned char)(0x80u | (_cp & 0x3Fu));
            break;

        case 3:
            bytes[0] = (unsigned char)(0xE0u | (_cp >> 12));
            bytes[1] = (unsigned char)(0x80u | ((_cp >> 6) & 0x3Fu));
            bytes[2] = (unsigned char)(0x80u | (_cp & 0x3Fu));
            break;

        case 4:
            bytes[0] = (unsigned char)(0xF0u | (_cp >> 18));
            bytes[1] = (unsigned char)(0x80u | ((_cp >> 12) & 0x3Fu));
            bytes[2] = (unsigned char)(0x80u | ((_cp >> 6) & 0x3Fu));
            bytes[3] = (unsigned char)(0x80u | (_cp & 0x3Fu));
            break;
    }

    return length;
}

#if D_THREAD_LOCAL_AVAILABLE

/*
d_text_emoji_utf8
  Validates before touching the buffer, so a refused code point leaves the
previous result intact. The buffer is a function-local static with
thread-local storage: each thread gets its own, and the pointer returned
stays valid until that thread calls again.
*/
const char*
d_text_emoji_utf8(
    uint32_t _cp
)
{
    const size_t length = d_internal_emoji_utf8_length(_cp);

    // not a scalar value: the previous result stays as it was
    if (length == 0)
    {
        return NULL;
    }

    // at most four bytes and the terminator
    static D_THREAD_LOCAL char buffer[5];

    (void)d_text_emoji_utf8_encode(_cp,
                                   buffer);
    buffer[length] = '\0';

    return buffer;
}

/*
d_text_emoji_utf8_seq
  Two passes. The first validates every code point and totals the bytes,
refusing the call as soon as the next code point and the terminator would no
longer fit; the second encodes. Nothing is written until the whole sequence
is known to fit, so a refused call leaves the previous result intact. The
running total never exceeds D_EMOJI_SEQ_CAPACITY - 1, so the room left,
D_EMOJI_SEQ_CAPACITY - total, cannot wrap.
*/
const char*
d_text_emoji_utf8_seq(
    const uint32_t* _cps,
    size_t          _length
)
{
    // a missing array is refused unless the sequence is empty
    if ( (!_cps) &&
         (_length != 0) )
    {
        return NULL;
    }

    size_t total = 0;

    // total the bytes, refusing a non-scalar value or an overflow
    for (size_t i = 0; i < _length; ++i)
    {
        const size_t length = d_internal_emoji_utf8_length(_cps[i]);

        // not a scalar value, or no room for it and the terminator
        if ( (length == 0) ||
             (length >= (size_t)D_EMOJI_SEQ_CAPACITY - total) )
        {
            return NULL;
        }

        total += length;
    }

    // one buffer per thread; the first pass showed everything fits
    static D_THREAD_LOCAL char buffer[D_EMOJI_SEQ_CAPACITY];

    size_t used = 0;

    // encode each code point after the one before it
    for (size_t i = 0; i < _length; ++i)
    {
        used += d_text_emoji_utf8_encode(_cps[i],
                                         buffer + used);
    }

    buffer[used] = '\0';

    return buffer;
}

#endif  // D_THREAD_LOCAL_AVAILABLE
