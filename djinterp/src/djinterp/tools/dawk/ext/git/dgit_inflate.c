/*******************************************************************************
* djinterp [tools]                                                dgit_inflate.c
*
* Definitions for the inflating of dgit_internal.h.
*   A zlib stream as RFC 1950 has it, around deflated blocks as RFC 1951 has
* them: stored, of the fixed codes, or of codes the block gives. Each code is
* canonical, so it is kept as its count of codes of each length and its
* symbols in order; a code of ten bits or fewer is found at once in a table
* indexed by the next ten bits of the stream, as they arrive least
* significant first, and a longer one bit by bit. The whole output is kept,
* so a distance reaches back into it directly; a stream whose length is
* known is held to it.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_inflate.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./dgit_internal.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memset
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint16_t,
                                                     // uint32_t, uint64_t


// D_INTERNAL_FAST
//   constant: how many bits the table finds a code by at once.
#define D_INTERNAL_FAST 10u

// D_INTERNAL_SYMBOLS
//   constant: the most symbols a code has: the literals and lengths' 288.
#define D_INTERNAL_SYMBOLS 288u

// d_internal_huffman
//   struct: a canonical code: how many codes of each length, its symbols
//   in the code's order, and the table of codes no longer than
//   D_INTERNAL_FAST bits -- each entry the code's length times 512 plus
//   its symbol, 0 where no short code begins.
struct d_internal_huffman
{
    uint16_t count[16];
    uint16_t symbol[D_INTERNAL_SYMBOLS];
    uint16_t fast[1u << D_INTERNAL_FAST];
};

// d_internal_inflate
//   struct: a stream being inflated: its bytes, the next to read, the bits
//   read and not yet used, how many bytes of zeros were read past its end
//   to fill them, and the output so far.
struct d_internal_inflate
{
    const uint8_t* in;
    size_t         length;
    size_t         at;
    uint64_t       bits;
    unsigned       count;
    size_t         past;
    uint8_t*       out;
    size_t         out_length;
    size_t         out_capacity;
    size_t         expected;
};

// d_internal_lengths, d_internal_length_bits, d_internal_distances,
// d_internal_distance_bits
//   constant: the base of each length and distance symbol, and the extra
//   bits after it.
static const uint16_t d_internal_lengths[29] =
{
    3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u, 15u, 17u, 19u, 23u, 27u, 31u,
    35u, 43u, 51u, 59u, 67u, 83u, 99u, 115u, 131u, 163u, 195u, 227u, 258u
};
static const uint8_t  d_internal_length_bits[29] =
{
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u, 2u, 2u, 2u, 2u,
    3u, 3u, 3u, 3u, 4u, 4u, 4u, 4u, 5u, 5u, 5u, 5u, 0u
};
static const uint16_t d_internal_distances[30] =
{
    1u, 2u, 3u, 4u, 5u, 7u, 9u, 13u, 17u, 25u, 33u, 49u, 65u, 97u, 129u,
    193u, 257u, 385u, 513u, 769u, 1025u, 1537u, 2049u, 3073u, 4097u, 6145u,
    8193u, 12289u, 16385u, 24577u
};
static const uint8_t  d_internal_distance_bits[30] =
{
    0u, 0u, 0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u, 4u, 4u, 5u, 5u, 6u, 6u,
    7u, 7u, 8u, 8u, 9u, 9u, 10u, 10u, 11u, 11u, 12u, 12u, 13u, 13u
};


/*
d_internal_fill
  Bits enough to hold `_need`, a byte at a time; past the stream's end,
zeros, counted, so a peek near the end still reads, and a use of them is
caught.
*/
static void
d_internal_fill(
    struct d_internal_inflate* _inflate,
    unsigned                   _need
)
{
    // a byte at a time, until there are enough bits
    while (_inflate->count < _need)
    {
        // a byte of the stream, or a zero past it
        if (_inflate->at < _inflate->length)
        {
            _inflate->bits |= (uint64_t)_inflate->in[_inflate->at]
                              << _inflate->count;
            ++_inflate->at;
        }
        else
        {
            ++_inflate->past;
        }

        _inflate->count += 8u;
    }

    return;
}


/*
d_internal_take
  The next `_need` bits, least significant first, used.
*/
static uint32_t
d_internal_take(
    struct d_internal_inflate* _inflate,
    unsigned                   _need
)
{
    // nothing to take
    if (_need == 0u)
    {
        return 0u;
    }

    d_internal_fill(_inflate, _need);

    const uint32_t value = (uint32_t)(_inflate->bits
                                      & ((1u << _need) - 1u));

    _inflate->bits  >>= _need;
    _inflate->count  -= _need;

    return value;
}


/*
d_internal_overrun
  Whether more bits were used than the stream holds.
*/
static bool
d_internal_overrun(
    const struct d_internal_inflate* _inflate
)
{
    return ((_inflate->past * 8u) > _inflate->count);
}


/*
d_internal_reverse
  A code's `_length` bits in the other order, as the stream holds them.
*/
static unsigned
d_internal_reverse(
    unsigned _code,
    unsigned _length
)
{
    unsigned reversed = 0u;

    // a bit at a time
    for (unsigned at = 0u; at < _length; ++at)
    {
        reversed = (reversed << 1) | ((_code >> at) & 1u);
    }

    return reversed;
}


/*
d_internal_build
  A canonical code from its symbols' lengths, as RFC 1951's 3.2.2 has it;
false for one over-subscribed, or incomplete but for a code of one symbol,
which deflate may give a block.
  Long as the codes are counted, numbered and tabled in one pass, as RFC
1951's 3.2.2 has it.
*/
static bool
d_internal_build(
    struct d_internal_huffman* _code,
    const uint8_t*             _lengths,
    unsigned                   _count
)
{
    uint16_t offsets[16];
    int      left = 1;

    memset(_code, 0, sizeof(*_code));

    // how many codes of each length
    for (unsigned at = 0u; at < _count; ++at)
    {
        ++_code->count[_lengths[at]];
    }

    // each length's codes within what the shorter left
    for (unsigned length = 1u; length < 16u; ++length)
    {
        left = (left * 2) - (int)_code->count[length];

        // more codes than can be
        if (left < 0)
        {
            return false;
        }
    }

    offsets[1] = 0u;

    // where each length's symbols begin
    for (unsigned length = 1u; length < 15u; ++length)
    {
        offsets[length + 1u] = (uint16_t)(offsets[length]
                                          + _code->count[length]);
    }

    unsigned codes   = 0u;
    unsigned code    = 0u;
    unsigned at_code = 0u;

    // the symbols, in the code's order
    for (unsigned at = 0u; at < _count; ++at)
    {
        // a symbol with a code
        if (_lengths[at] != 0u)
        {
            _code->symbol[offsets[_lengths[at]]++] = (uint16_t)at;
            ++codes;
        }
    }

    // the fast table, from each code of D_INTERNAL_FAST bits or fewer
    for (unsigned length = 1u; length <= D_INTERNAL_FAST; ++length)
    {
        // each code of the length, in order
        for (unsigned n = 0u; n < _code->count[length]; ++n, ++code, ++at_code)
        {
            const unsigned reversed = d_internal_reverse(code, length);

            // every index whose low bits are the code
            for (unsigned fill = reversed;
                 fill < (1u << D_INTERNAL_FAST);
                 fill += (1u << length))
            {
                _code->fast[fill] = (uint16_t)((length << 9)
                                               | _code->symbol[at_code]);
            }
        }

        code <<= 1;
    }

    return ( (left == 0) ||
             (codes <= 1u) );
}


/*
d_internal_slow
  A code longer than the table holds, bit by bit, as RFC 1951's canonical
order lets it be read: at each length, the codes of that length are the
next in order. -1 for bits that are no code.
*/
static int
d_internal_slow(
    struct d_internal_inflate*       _inflate,
    const struct d_internal_huffman* _code
)
{
    int code  = 0;
    int first = 0;
    int index = 0;

    // a bit at a time, the codes of each length in turn
    for (unsigned length = 1u; length < 16u; ++length)
    {
        code |= (int)d_internal_take(_inflate, 1u);

        const int count = _code->count[length];

        // a code of this length
        if ((code - count) < first)
        {
            return _code->symbol[index + (code - first)];
        }

        index += count;
        first += count;
        first <<= 1;
        code  <<= 1;
    }

    return -1;
}


/*
d_internal_decode
  The next symbol of a code: from the table when its code is short, bit by
bit when not.
*/
static int
d_internal_decode(
    struct d_internal_inflate*       _inflate,
    const struct d_internal_huffman* _code
)
{
    d_internal_fill(_inflate, D_INTERNAL_FAST);

    const unsigned entry = _code->fast[_inflate->bits
                                       & ((1u << D_INTERNAL_FAST) - 1u)];

    // a short code, found at once
    if (entry != 0u)
    {
        const unsigned length = entry >> 9;

        _inflate->bits  >>= length;
        _inflate->count  -= length;

        return (int)(entry & 511u);
    }

    return d_internal_slow(_inflate, _code);
}


/*
d_internal_room
  Room for `_more` bytes of output, but not past a length that is known.
*/
static bool
d_internal_room(
    struct d_internal_inflate* _inflate,
    size_t                     _more
)
{
    const size_t wanted = _inflate->out_length + _more;

    // past what the stream is known to inflate to
    if ( (_inflate->expected > 0u) &&
         (wanted > _inflate->expected) )
    {
        return false;
    }

    // room already
    if (wanted <= _inflate->out_capacity)
    {
        return true;
    }

    size_t capacity = (_inflate->out_capacity > 0u)
                      ? _inflate->out_capacity
                      : 4096u;

    // doubled until it fits
    while (capacity < wanted)
    {
        capacity *= 2u;
    }

    uint8_t* const grown = realloc(_inflate->out, capacity);

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    _inflate->out          = grown;
    _inflate->out_capacity = capacity;

    return true;
}


/*
d_internal_stored
  A stored block: to a byte's boundary, its length and that length's
complement, then that many bytes as they are.
*/
static bool
d_internal_stored(
    struct d_internal_inflate* _inflate
)
{
    // to a byte's boundary
    (void)d_internal_take(_inflate, _inflate->count % 8u);

    const uint32_t length     = d_internal_take(_inflate, 16u);
    const uint32_t complement = d_internal_take(_inflate, 16u);

    // the length and its complement agree, and the bytes are there
    if ( (length != (~complement & 0xFFFFu)) ||
         (d_internal_overrun(_inflate)) ||
         (!d_internal_room(_inflate, length)) )
    {
        return false;
    }

    // the bytes, through what the bit reader holds and then the stream
    for (uint32_t at = 0u; at < length; ++at)
    {
        _inflate->out[_inflate->out_length] =
            (uint8_t)d_internal_take(_inflate, 8u);
        ++_inflate->out_length;
    }

    return !d_internal_overrun(_inflate);
}


/*
d_internal_copy_back
  A length and distance: bytes copied from that far back in the output,
the copy reaching into what it writes when the distance is the shorter.
*/
static bool
d_internal_copy_back(
    struct d_internal_inflate* _inflate,
    size_t                     _length,
    size_t                     _distance
)
{
    // a distance within the output, and room for the copy
    if ( (_distance > _inflate->out_length) ||
         (!d_internal_room(_inflate, _length)) )
    {
        return false;
    }

    uint8_t* const out  = _inflate->out;
    size_t         to   = _inflate->out_length;
    size_t         from = to - _distance;

    // a byte at a time, so the copy may overlap itself
    for (size_t at = 0u; at < _length; ++at)
    {
        out[to++] = out[from++];
    }

    _inflate->out_length = to;

    return true;
}


/*
d_internal_codes
  A block's literals, lengths and distances, until its end symbol.
*/
static bool
d_internal_codes(
    struct d_internal_inflate*       _inflate,
    const struct d_internal_huffman* _literals,
    const struct d_internal_huffman* _distances
)
{
    // each symbol, until the block's end
    for (;;)
    {
        const int symbol = d_internal_decode(_inflate, _literals);

        // a literal byte, the end, or a length
        if ( (symbol < 0) ||
             (d_internal_overrun(_inflate)) )
        {
            return false;
        }
        else if (symbol < 256)
        {
            // room for it
            if (!d_internal_room(_inflate, 1u))
            {
                return false;
            }

            _inflate->out[_inflate->out_length] = (uint8_t)symbol;
            ++_inflate->out_length;

            continue;
        }
        else if (symbol == 256)
        {
            return true;
        }
        else if (symbol > 285)
        {
            return false;
        }

        const unsigned lsym   = (unsigned)symbol - 257u;
        const size_t   length = d_internal_lengths[lsym]
                                + d_internal_take(_inflate,
                                                  d_internal_length_bits[lsym]);
        const int      dsym   = d_internal_decode(_inflate, _distances);

        // a distance symbol that is one
        if ( (dsym < 0) ||
             (dsym > 29) )
        {
            return false;
        }

        const size_t distance = d_internal_distances[dsym]
                                + d_internal_take(_inflate,
                                       d_internal_distance_bits[dsym]);

        // the copy
        if (!d_internal_copy_back(_inflate, length, distance))
        {
            return false;
        }
    }
}


/*
d_internal_fixed
  A block of the fixed codes RFC 1951's 3.2.6 gives.
*/
static bool
d_internal_fixed(
    struct d_internal_inflate* _inflate
)
{
    static struct d_internal_huffman literals;
    static struct d_internal_huffman distances;
    static bool                      built = false;

    // the codes, built once
    if (!built)
    {
        uint8_t lengths[D_INTERNAL_SYMBOLS];

        // the literals' and lengths' lengths, by range
        for (unsigned at = 0u; at < D_INTERNAL_SYMBOLS; ++at)
        {
            lengths[at] = (at < 144u) ? 8u
                        : (at < 256u) ? 9u
                        : (at < 280u) ? 7u
                        : 8u;
        }

        (void)d_internal_build(&literals, lengths, D_INTERNAL_SYMBOLS);
        memset(lengths, 5, 30u);
        (void)d_internal_build(&distances, lengths, 30u);
        built = true;
    }

    return d_internal_codes(_inflate, &literals, &distances);
}


/*
d_internal_dynamic_lengths
  The lengths a dynamic block gives its codes, read by the code of code
lengths: 0 to 15 a length, 16 the last repeated, 17 and 18 runs of zeros.
*/
static bool
d_internal_dynamic_lengths(
    struct d_internal_inflate*       _inflate,
    const struct d_internal_huffman* _lengths_code,
    uint8_t*                         _lengths,
    unsigned                         _count
)
{
    unsigned at = 0u;

    // until every symbol has its length
    while (at < _count)
    {
        const int symbol = d_internal_decode(_inflate, _lengths_code);

        // a length, or a repeat of one
        if (symbol < 0)
        {
            return false;
        }
        else if (symbol < 16)
        {
            _lengths[at] = (uint8_t)symbol;
            ++at;

            continue;
        }

        // a repeat of the last, or of zero
        const bool     last   = (symbol == 16);
        const unsigned repeat = (symbol == 16)
                                ? (3u + d_internal_take(_inflate, 2u))
                                : ( (symbol == 17)
                                    ? (3u + d_internal_take(_inflate, 3u))
                                    : (11u + d_internal_take(_inflate, 7u)) );

        // a repeat of the last needs one, and none runs past the end
        if ( ( (last) && (at == 0u) ) ||
             ((at + repeat) > _count) )
        {
            return false;
        }

        const uint8_t value = (last) ? _lengths[at - 1u] : 0u;

        // the run
        for (unsigned n = 0u; n < repeat; ++n)
        {
            _lengths[at] = value;
            ++at;
        }
    }

    return !d_internal_overrun(_inflate);
}


/*
d_internal_dynamic
  A block of the codes it gives: their counts, the code of code lengths in
its odd order, the lengths, and then the block.
*/
static bool
d_internal_dynamic(
    struct d_internal_inflate* _inflate
)
{
    static const uint8_t order[19] = { 16u, 17u, 18u, 0u, 8u, 7u, 9u, 6u, 10u,
                                       5u, 11u, 4u, 12u, 3u, 13u, 2u, 14u, 1u,
                                       15u };
    struct d_internal_huffman literals;
    struct d_internal_huffman distances;
    uint8_t                   lengths[D_INTERNAL_SYMBOLS + 32u];
    const unsigned            nlen  = d_internal_take(_inflate, 5u) + 257u;
    const unsigned            ndist = d_internal_take(_inflate, 5u) + 1u;
    const unsigned            ncode = d_internal_take(_inflate, 4u) + 4u;

    // counts that are within the codes
    if ( (nlen > 286u) ||
         (ndist > 30u) )
    {
        return false;
    }

    memset(lengths, 0, sizeof(lengths));

    // the code of code lengths' lengths, in their order
    for (unsigned at = 0u; at < ncode; ++at)
    {
        lengths[order[at]] = (uint8_t)d_internal_take(_inflate, 3u);
    }

    // the code of code lengths, then the lengths it gives
    if ( (!d_internal_build(&literals, lengths, 19u)) ||
         (!d_internal_dynamic_lengths(_inflate,
                                      &literals,
                                      lengths,
                                      nlen + ndist)) )
    {
        return false;
    }

    // a block must have an end, and its codes must be sound
    if ( (lengths[256] == 0u) ||
         (!d_internal_build(&literals, lengths, nlen)) ||
         (!d_internal_build(&distances, &lengths[nlen], ndist)) )
    {
        return false;
    }

    return d_internal_codes(_inflate, &literals, &distances);
}


/*
d_internal_adler
  The Adler-32 of bytes, as zlib's trailer holds it.
*/
static uint32_t
d_internal_adler(
    const uint8_t* _bytes,
    size_t         _length
)
{
    uint32_t a = 1u;
    uint32_t b = 0u;

    // in runs short enough that the sums cannot overflow before reduction
    while (_length > 0u)
    {
        const size_t run = (_length < 5552u) ? _length : 5552u;

        // the run's bytes
        for (size_t at = 0u; at < run; ++at)
        {
            a += _bytes[at];
            b += a;
        }

        a       %= 65521u;
        b       %= 65521u;
        _bytes  += run;
        _length -= run;
    }

    return (b << 16) | a;
}


/*
d_internal_blocks
  Every block, to the one marked last.
*/
static bool
d_internal_blocks(
    struct d_internal_inflate* _inflate
)
{
    bool last = false;

    // a block at a time
    while (!last)
    {
        last = (d_internal_take(_inflate, 1u) != 0u);

        const uint32_t type = d_internal_take(_inflate, 2u);
        const bool     read = (type == 0u) ? d_internal_stored(_inflate)
                            : (type == 1u) ? d_internal_fixed(_inflate)
                            : (type == 2u) ? d_internal_dynamic(_inflate)
                            : false;

        // a block that did not read
        if ( (!read) ||
             (d_internal_overrun(_inflate)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_git_inflate
  The header's method must be deflate's, with a window of 32 KiB or less,
no preset dictionary, and a check that holds. After the last block, to a
byte's boundary, the trailer is the output's Adler-32, big-endian; the
bytes used are those the reader took less those it holds unused.
*/
bool
d_git_inflate(
    const uint8_t*         _stream,
    size_t                 _length,
    size_t                 _expected,
    struct d_git_inflated* _out
)
{
    struct d_internal_inflate inflate;

    memset(_out, 0, sizeof(*_out));
    memset(&inflate, 0, sizeof(inflate));
    inflate.in       = _stream;
    inflate.length   = _length;
    inflate.expected = _expected;

    // a header: deflate, its window, no dictionary, and its check
    if ( (_length < 6u)                                          ||
         ((_stream[0] & 0x0Fu) != 8u)                            ||
         ((_stream[0] >> 4) > 7u)                                ||
         ((_stream[1] & 0x20u) != 0u)                            ||
         ((((unsigned)_stream[0] << 8) | _stream[1]) % 31u != 0u) )
    {
        return false;
    }

    inflate.at = 2u;

    const bool read = d_internal_blocks(&inflate);

    // to a byte's boundary, then the trailer
    (void)d_internal_take(&inflate, inflate.count % 8u);

    const uint32_t adler = (d_internal_take(&inflate, 8u) << 24)
                           | (d_internal_take(&inflate, 8u) << 16)
                           | (d_internal_take(&inflate, 8u) << 8)
                           | d_internal_take(&inflate, 8u);

    // the output terminated, as an object's content is: a NUL past it,
    // which it does not count
    uint8_t* const ended = (inflate.out_length < inflate.out_capacity)
                           ? inflate.out
                           : realloc(inflate.out, inflate.out_length + 1u);
    const bool     room  = (ended != NULL);

    // room for the NUL, or what there was kept to free
    if (room)
    {
        inflate.out                     = ended;
        inflate.out_capacity            = (inflate.out_length
                                           < inflate.out_capacity)
                                          ? inflate.out_capacity
                                          : (inflate.out_length + 1u);
        inflate.out[inflate.out_length] = 0u;
    }

    _out->bytes  = inflate.out;
    _out->length = inflate.out_length;
    _out->used   = (inflate.at + inflate.past) - (inflate.count / 8u);

    return ( (read)                                               &&
             (room)                                               &&
             (!d_internal_overrun(&inflate))                       &&
             ( (_expected == 0u) ||
               (inflate.out_length == _expected) )                &&
             (adler == d_internal_adler(inflate.out,
                                        inflate.out_length)) );
}
