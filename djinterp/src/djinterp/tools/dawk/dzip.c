/*******************************************************************************
* djinterp [tools]                                                        dzip.c
*
* Definitions for the declarations in dzip.h.
*   The directory at the end of the archive is the truth about it: the local
* header before each entry's data repeats some of what the directory says,
* and may say less of it -- an entry streamed with a data descriptor has zeros
* there -- so sizes and checksums are taken from the directory and the local
* header is read only for where the data begins.
*   Inflate is RFC 1951 decoded a bit at a time through canonical Huffman
* tables, as Mark Adler's puff does it: slower than zlib, and short enough to
* check against the RFC line by line. The entries it reads here are XML parts,
* which are small; media are usually stored, and are copied, not inflated.
*
* path:      /src/djinterp/tools/dawk/dzip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dzip.h"  // corresponding header
// std
#include <limits.h>  // LONG_MAX
#include <stdio.h>   // FILE, fopen, fread, fwrite, fseek, ftell, snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free
#include <string.h>  // memcpy, memset, strcmp, strlen


//==============================================================================
// 1.  INTERNAL TYPES AND CONSTANTS
//==============================================================================


// 1.1    Records
//------------------------------------------------------------------------------
// D_INTERNAL_ZIP_LOCAL, D_INTERNAL_ZIP_CENTRAL, D_INTERNAL_ZIP_END
//   constant: the signatures of a local header, a directory record and the
// end of the directory, as the four little-endian bytes "PK\3\4", "PK\1\2"
// and "PK\5\6" read.
#define D_INTERNAL_ZIP_LOCAL   0x04034b50ul
#define D_INTERNAL_ZIP_CENTRAL 0x02014b50ul
#define D_INTERNAL_ZIP_END     0x06054b50ul

// D_INTERNAL_ZIP_END_MAX
//   constant: how far from the end the end record can start: its 22 bytes and
// a comment of at most 65535.
#define D_INTERNAL_ZIP_END_MAX (22u + 65535u)

// D_INTERNAL_ZIP_UTF8
//   constant: the flag saying a name is UTF-8, the one flag a rewritten entry
// keeps; the data-descriptor flag is dropped, since sizes are written into
// the local header.
#define D_INTERNAL_ZIP_UTF8 0x0800u

// 1.2    Entries
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_zip_entry
//   struct: one entry, as the directory describes it, and its replacement.
//   `bytes` is NULL for an entry copied from the archive as it was stored.
struct d_internal_zip_entry
{
    char*          name;
    unsigned       version_made;
    unsigned       version_needed;
    unsigned       flags;
    unsigned       method;
    unsigned       time;
    unsigned       date;
    unsigned long  crc;
    unsigned long  compressed;
    unsigned long  size;
    unsigned       internal_attributes;
    unsigned long  external_attributes;
    unsigned long  offset;
    unsigned char* bytes;
    size_t         length;
};

// 1.3    Archive
//------------------------------------------------------------------------------
// 1.3.1
// d_zip
//   struct: the open file, if any, and the entries.
struct d_zip
{
    FILE*                        file;
    char*                        path;
    long                         file_size;
    struct d_internal_zip_entry* entries;
    size_t                       count;
    size_t                       capacity;
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/*
d_internal_problem
  Writes a reason into a caller's buffer, which may be NULL; always false, so
a failing path can return it.
*/
static bool
d_internal_problem(
    char*       _problem,
    size_t      _capacity,
    const char* _message,
    const char* _subject
)
{
    // a caller that did not ask for the reason is given none
    if ( (_problem) &&
         (_capacity > 0) )
    {
        (void)snprintf(_problem,
                       _capacity,
                       (_subject) ? "%s: %s" : "%s",
                       _message,
                       _subject);
    }

    return false;
}


/*
d_internal_le16, d_internal_le32
  Read a little-endian 16- or 32-bit field, ZIP's only byte order.
*/
static unsigned
d_internal_le16(
    const unsigned char* _at
)
{
    return (unsigned)_at[0] | ((unsigned)_at[1] << 8);
}


static unsigned long
d_internal_le32(
    const unsigned char* _at
)
{
    return  (unsigned long)_at[0]          |
           ((unsigned long)_at[1] << 8)    |
           ((unsigned long)_at[2] << 16)   |
           ((unsigned long)_at[3] << 24);
}


/*
d_internal_put16, d_internal_put32
  Write a little-endian field into a record being built.
*/
static void
d_internal_put16(
    unsigned char* _at,
    unsigned long  _value
)
{
    _at[0] = (unsigned char)(_value & 0xFFu);
    _at[1] = (unsigned char)((_value >> 8) & 0xFFu);

    return;
}


static void
d_internal_put32(
    unsigned char* _at,
    unsigned long  _value
)
{
    d_internal_put16(_at, _value & 0xFFFFul);
    d_internal_put16(&_at[2], (_value >> 16) & 0xFFFFul);

    return;
}


/*
d_internal_read_at
  Reads exactly `_length` bytes from an offset of the archive's file.
*/
static bool
d_internal_read_at(
    struct d_zip*  _zip,
    unsigned long  _offset,
    unsigned char* _out,
    size_t         _length
)
{
    // an offset past what fseek can reach, or past the file, is no data
    if ( (_offset > (unsigned long)LONG_MAX)      ||
         ((long)_offset > _zip->file_size) )
    {
        return false;
    }

    return ( (fseek(_zip->file, (long)_offset, SEEK_SET) == 0) &&
             (fread(_out, 1u, _length, _zip->file) == _length) );
}


/*
d_internal_grow_entries
  Makes room for one more entry.
*/
static bool
d_internal_grow_entries(
    struct d_zip* _zip
)
{
    // a table with room left is left as it is
    if (_zip->count < _zip->capacity)
    {
        return true;
    }

    const size_t capacity = (_zip->capacity == 0) ? 16u : (_zip->capacity * 2u);

    struct d_internal_zip_entry* const grown =
        realloc(_zip->entries, capacity * sizeof(*grown));

    // report the failure rather than dropping an entry
    if (!grown)
    {
        return false;
    }

    _zip->entries  = grown;
    _zip->capacity = capacity;

    return true;
}


/*
d_internal_copy_text
  Copies a run of bytes into an allocated, terminated string.
*/
static char*
d_internal_copy_text(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // a copy that cannot be held is reported by its absence
    if (!copy)
    {
        return NULL;
    }

    memcpy(copy, _text, _length);
    copy[_length] = '\0';

    return copy;
}


//==============================================================================
// 3.  CRC-32
//==============================================================================


/*
d_zip_crc32
  Four bits at a time from a sixteen-entry table, which needs no setup and no
writable state: fast enough for files read from disk, which are read slower.
*/
unsigned long
d_zip_crc32(
    unsigned long        _crc,
    const unsigned char* _bytes,
    size_t               _length
)
{
    static const unsigned long nibbles[16] =
    {
        0x00000000ul, 0x1DB71064ul, 0x3B6E20C8ul, 0x26D930ACul,
        0x76DC4190ul, 0x6B6B51F4ul, 0x4DB26158ul, 0x5005713Cul,
        0xEDB88320ul, 0xF00F9344ul, 0xD6D6A3E8ul, 0xCB61B38Cul,
        0x9B64C2B0ul, 0x86D3D2D4ul, 0xA00AE278ul, 0xBDBDF21Cul
    };

    unsigned long crc = (~_crc) & 0xFFFFFFFFul;

    for (size_t at = 0; at < _length; ++at)
    {
        crc ^= (unsigned long)_bytes[at];
        crc  = nibbles[crc & 0x0Fu] ^ (crc >> 4);
        crc  = nibbles[crc & 0x0Fu] ^ (crc >> 4);
    }

    return (~crc) & 0xFFFFFFFFul;
}


//==============================================================================
// 4.  INFLATE
//==============================================================================


// 4.1    State
//------------------------------------------------------------------------------
// D_INTERNAL_MAX_BITS, D_INTERNAL_MAX_LITERALS, D_INTERNAL_MAX_DISTANCES
//   constant: the longest code, and the most literal/length and distance
// codes a dynamic block can define, as RFC 1951 fixes them.
#define D_INTERNAL_MAX_BITS      15
#define D_INTERNAL_MAX_LITERALS  286
#define D_INTERNAL_MAX_DISTANCES 30

// D_INTERNAL_FIXED_LITERALS
//   constant: the literal/length codes the fixed code assigns lengths to.
// Two of them, 286 and 287, never occur, but the code is built over all 288:
// built over 286, it would be incomplete in a way that moves every 9-bit
// code, and literals 144 to 255 would decode as the wrong bytes.
#define D_INTERNAL_FIXED_LITERALS 288

// 4.1.1
// d_internal_inflate
//   struct: the input and its bit buffer, and the output.  `failed` is set by
//   the first read past the input or write past the output, after which every
//   read yields zeros and the decode unwinds.
struct d_internal_inflate
{
    const unsigned char* in;
    size_t               in_length;
    size_t               in_at;
    unsigned long        bits;
    int                  bit_count;
    unsigned char*       out;
    size_t               out_length;
    size_t               out_at;
    bool                 failed;
};

// 4.1.2
// d_internal_huffman
//   struct: a canonical Huffman code, as how many codes there are of each
//   length and the symbols in code order.
struct d_internal_huffman
{
    short count[D_INTERNAL_MAX_BITS + 1];
    short symbol[D_INTERNAL_FIXED_LITERALS];
};

// 4.2    Decoding
//------------------------------------------------------------------------------
/*
d_internal_bits
  Takes `_need` bits from the input, least significant first, as DEFLATE packs
them.
*/
static unsigned
d_internal_bits(
    struct d_internal_inflate* _s,
    int                        _need
)
{
    // the bit buffer is filled a byte at a time until it holds enough
    while (_s->bit_count < _need)
    {
        // input that ends inside a block is a damaged stream
        if (_s->in_at >= _s->in_length)
        {
            _s->failed = true;

            return 0;
        }

        _s->bits      |= (unsigned long)_s->in[_s->in_at++] << _s->bit_count;
        _s->bit_count += 8;
    }

    const unsigned value = (unsigned)(_s->bits & ((1ul << _need) - 1ul));

    _s->bits      >>= _need;
    _s->bit_count  -= _need;

    return value;
}


/*
d_internal_put
  Appends one byte to the output, which is exactly as long as the directory
says the entry is: a stream that would write past it is damaged.
*/
static void
d_internal_put(
    struct d_internal_inflate* _s,
    unsigned char              _byte
)
{
    // a byte past the expected size means the stream and the directory differ
    if (_s->out_at >= _s->out_length)
    {
        _s->failed = true;

        return;
    }

    _s->out[_s->out_at++] = _byte;

    return;
}


/*
d_internal_huffman_build
  Builds a canonical code from each symbol's code length, as RFC 1951 3.2.2
assigns codes.
Return:
  0 for a complete code, a positive number for an incomplete one, which a
single-distance-code block may legitimately have, and -1 for an
over-subscribed one, which no stream can mean.
*/
static int
d_internal_huffman_build(
    struct d_internal_huffman* _h,
    const short*               _lengths,
    int                        _count
)
{
    short offsets[D_INTERNAL_MAX_BITS + 1];

    memset(_h->count, 0, sizeof(_h->count));

    // count the codes of each length
    for (int symbol = 0; symbol < _count; ++symbol)
    {
        _h->count[_lengths[symbol]]++;
    }

    // a code of no symbols at all is complete, and decodes nothing
    if (_h->count[0] == _count)
    {
        return 0;
    }

    int left = 1;

    // each length doubles the codes available; more used than there are is
    // an over-subscribed code
    for (int length = 1; length <= D_INTERNAL_MAX_BITS; ++length)
    {
        left <<= 1;
        left  -= _h->count[length];

        if (left < 0)
        {
            return -1;
        }
    }

    offsets[1] = 0;

    // the first symbol of each length follows those of the shorter ones
    for (int length = 1; length < D_INTERNAL_MAX_BITS; ++length)
    {
        offsets[length + 1] = (short)(offsets[length] + _h->count[length]);
    }

    // the symbols, in code order
    for (int symbol = 0; symbol < _count; ++symbol)
    {
        if (_lengths[symbol] != 0)
        {
            _h->symbol[offsets[_lengths[symbol]]++] = (short)symbol;
        }
    }

    return left;
}


/*
d_internal_decode
  Decodes one symbol, a bit at a time: the code read so far is compared with
the first code of each length until it falls among them.
Return:
  The symbol, or -1 for a code the table does not hold.
*/
static int
d_internal_decode(
    struct d_internal_inflate*       _s,
    const struct d_internal_huffman* _h
)
{
    int code  = 0;
    int first = 0;
    int index = 0;

    for (int length = 1; length <= D_INTERNAL_MAX_BITS; ++length)
    {
        code |= (int)d_internal_bits(_s, 1);

        const int count = _h->count[length];

        // a code below the first of the next length is one of this length
        if ((code - count) < first)
        {
            return _h->symbol[index + (code - first)];
        }

        index  += count;
        first  += count;
        first <<= 1;
        code  <<= 1;
    }

    return -1;
}


/*
d_internal_codes
  Decodes the literals, lengths and distances of one compressed block, up to
its end-of-block code.
Return:
  true at the end of the block; false for a damaged stream.
*/
static bool
d_internal_codes(
    struct d_internal_inflate*       _s,
    const struct d_internal_huffman* _literals,
    const struct d_internal_huffman* _distances
)
{
    static const short length_base[29] =
    {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static const short length_extra[29] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    static const short distance_base[30] =
    {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
    };
    static const short distance_extra[30] =
    {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };

    // symbols are decoded until the end of the block or a fault
    while (!_s->failed)
    {
        const int symbol = d_internal_decode(_s, _literals);

        // a code the table does not hold is a damaged stream
        if (symbol < 0)
        {
            return false;
        }

        // a literal is itself
        if (symbol < 256)
        {
            d_internal_put(_s, (unsigned char)symbol);
            continue;
        }

        // 256 ends the block
        if (symbol == 256)
        {
            return true;
        }

        const int length_code = symbol - 257;

        // a length code past the table is a damaged stream
        if (length_code >= 29)
        {
            return false;
        }

        const size_t length = (size_t)length_base[length_code] +
                              d_internal_bits(_s, length_extra[length_code]);
        const int    distance_code = d_internal_decode(_s, _distances);

        // a distance code past the table is a damaged stream
        if ( (distance_code < 0) ||
             (distance_code >= 30) )
        {
            return false;
        }

        const size_t distance =
            (size_t)distance_base[distance_code] +
            d_internal_bits(_s, distance_extra[distance_code]);

        // a distance reaching before the output began is a damaged stream
        if (distance > _s->out_at)
        {
            return false;
        }

        // a copy may overlap itself, which is how runs are written
        for (size_t at = 0; (at < length) && (!_s->failed); ++at)
        {
            d_internal_put(_s, _s->out[_s->out_at - distance]);
        }
    }

    return false;
}


/*
d_internal_stored
  Copies a stored block, which begins at the next byte boundary.
*/
static bool
d_internal_stored(
    struct d_internal_inflate* _s
)
{
    // the bits left of the current byte are padding
    _s->bits      = 0;
    _s->bit_count = 0;

    // the block's length and its complement are four bytes
    if ((_s->in_at + 4u) > _s->in_length)
    {
        return false;
    }

    const unsigned length     = d_internal_le16(&_s->in[_s->in_at]);
    const unsigned complement = d_internal_le16(&_s->in[_s->in_at + 2u]);

    _s->in_at += 4u;

    // a length its complement does not confirm is a damaged stream
    if ( (length != (~complement & 0xFFFFu))   ||
         ((_s->in_at + length) > _s->in_length) )
    {
        return false;
    }

    for (unsigned at = 0; (at < length) && (!_s->failed); ++at)
    {
        d_internal_put(_s, _s->in[_s->in_at++]);
    }

    return !_s->failed;
}


/*
d_internal_fixed
  Decodes a block with the fixed codes of RFC 1951 3.2.6.
*/
static bool
d_internal_fixed(
    struct d_internal_inflate* _s
)
{
    struct d_internal_huffman literals;
    struct d_internal_huffman distances;
    short                     lengths[D_INTERNAL_FIXED_LITERALS];

    // the literal/length code: 8, 9, 7 and 8 bits over its four ranges
    for (int symbol = 0; symbol < D_INTERNAL_FIXED_LITERALS; ++symbol)
    {
        lengths[symbol] = (symbol < 144) ? 8
                        : (symbol < 256) ? 9
                        : (symbol < 280) ? 7
                        :                  8;
    }

    (void)d_internal_huffman_build(&literals,
                                   lengths,
                                   D_INTERNAL_FIXED_LITERALS);

    // the distance code: five bits for each of its thirty symbols
    for (int symbol = 0; symbol < D_INTERNAL_MAX_DISTANCES; ++symbol)
    {
        lengths[symbol] = 5;
    }

    (void)d_internal_huffman_build(&distances,
                                   lengths,
                                   D_INTERNAL_MAX_DISTANCES);

    return d_internal_codes(_s, &literals, &distances);
}


/*
d_internal_dynamic
  Decodes a block that carries its own codes, themselves coded with a code
of code lengths, as RFC 1951 3.2.7 describes.
*/
static bool
d_internal_dynamic(
    struct d_internal_inflate* _s
)
{
    static const short order[19] =
    {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };

    const int literal_count  = (int)d_internal_bits(_s, 5) + 257;
    const int distance_count = (int)d_internal_bits(_s, 5) + 1;
    const int length_count   = (int)d_internal_bits(_s, 4) + 4;

    // more codes than DEFLATE defines is a damaged stream
    if ( (literal_count > D_INTERNAL_MAX_LITERALS)    ||
         (distance_count > D_INTERNAL_MAX_DISTANCES) )
    {
        return false;
    }

    short                     lengths[D_INTERNAL_MAX_LITERALS +
                                      D_INTERNAL_MAX_DISTANCES];
    struct d_internal_huffman code_lengths;

    // the code-length code's lengths come three bits each, in a fixed order
    for (int at = 0; at < 19; ++at)
    {
        lengths[order[at]] = (at < length_count)
                           ? (short)d_internal_bits(_s, 3)
                           : (short)0;
    }

    // the code-length code must be complete
    if (d_internal_huffman_build(&code_lengths, lengths, 19) != 0)
    {
        return false;
    }

    int at = 0;

    // the literal and distance lengths, with runs coded by 16, 17 and 18
    while ( (at < (literal_count + distance_count)) &&
            (!_s->failed) )
    {
        const int symbol = d_internal_decode(_s, &code_lengths);

        // a code the table does not hold is a damaged stream
        if (symbol < 0)
        {
            return false;
        }

        // 0 to 15 is a length itself
        if (symbol < 16)
        {
            lengths[at++] = (short)symbol;
            continue;
        }

        short repeat_length = 0;
        int   repeat        = 0;

        // 16 repeats the previous length; 17 and 18 repeat zero
        if (symbol == 16)
        {
            // there is no previous length to repeat before the first
            if (at == 0)
            {
                return false;
            }

            repeat_length = lengths[at - 1];
            repeat        = 3 + (int)d_internal_bits(_s, 2);
        }
        else if (symbol == 17)
        {
            repeat = 3 + (int)d_internal_bits(_s, 3);
        }
        else
        {
            repeat = 11 + (int)d_internal_bits(_s, 7);
        }

        // a run past the lengths the block declared is a damaged stream
        if ((at + repeat) > (literal_count + distance_count))
        {
            return false;
        }

        while (repeat-- > 0)
        {
            lengths[at++] = repeat_length;
        }
    }

    // a block without an end-of-block code could never end
    if ( (_s->failed) ||
         (lengths[256] == 0) )
    {
        return false;
    }

    struct d_internal_huffman literals;
    struct d_internal_huffman distances;
    const int                 literal_state =
        d_internal_huffman_build(&literals, lengths, literal_count);

    // an incomplete literal code is allowed only as a single code
    if ( (literal_state < 0) ||
         ( (literal_state > 0) &&
           ((literal_count - literals.count[0]) != 1) ) )
    {
        return false;
    }

    const int distance_state = d_internal_huffman_build(&distances,
                                                        &lengths[literal_count],
                                                        distance_count);

    // likewise an incomplete distance code
    if ( (distance_state < 0) ||
         ( (distance_state > 0) &&
           ((distance_count - distances.count[0]) != 1) ) )
    {
        return false;
    }

    return d_internal_codes(_s, &literals, &distances);
}


/*
d_internal_inflate_all
  Inflates a whole raw DEFLATE stream into an output of exactly the size the
directory gives, block by block until the last.
*/
static bool
d_internal_inflate_all(
    const unsigned char* _in,
    size_t               _in_length,
    unsigned char*       _out,
    size_t               _out_length
)
{
    struct d_internal_inflate s;

    memset(&s, 0, sizeof(s));
    s.in         = _in;
    s.in_length  = _in_length;
    s.out        = _out;
    s.out_length = _out_length;

    bool last = false;

    // blocks follow one another until the one marked last
    while (!last)
    {
        last = (d_internal_bits(&s, 1) == 1u);

        const unsigned type = d_internal_bits(&s, 2);
        bool           ok   = false;

        // the block type decides how the block is coded; 3 is reserved
        if (s.failed)
        {
            ok = false;
        }
        else if (type == 0u)
        {
            ok = d_internal_stored(&s);
        }
        else if (type == 1u)
        {
            ok = d_internal_fixed(&s);
        }
        else if (type == 2u)
        {
            ok = d_internal_dynamic(&s);
        }

        // a damaged block ends the stream
        if ( (!ok) ||
             (s.failed) )
        {
            return false;
        }
    }

    // a stream that ends short of the entry's size is damaged too
    return (s.out_at == s.out_length);
}


//==============================================================================
// 5.  READING THE DIRECTORY
//==============================================================================


/*
d_internal_find_end
  Finds the end-of-directory record, searching back from the end of the file
over the comment it may carry.
*/
static bool
d_internal_find_end(
    struct d_zip*  _zip,
    unsigned char* _record
)
{
    const long window = (_zip->file_size < (long)D_INTERNAL_ZIP_END_MAX)
                      ? _zip->file_size
                      : (long)D_INTERNAL_ZIP_END_MAX;

    // a file shorter than an end record is no archive
    if (window < 22)
    {
        return false;
    }

    unsigned char* const tail = malloc((size_t)window);

    // report the failure as the absence of a record
    if (!tail)
    {
        return false;
    }

    bool found = d_internal_read_at(_zip,
                                    (unsigned long)(_zip->file_size - window),
                                    tail,
                                    (size_t)window);

    // the last signature in the window is the record; a comment may hold
    // the signature's bytes, so the search starts from the end
    for (long at = window - 22; (found) && (at >= 0); --at)
    {
        if (d_internal_le32(&tail[at]) == D_INTERNAL_ZIP_END)
        {
            memcpy(_record, &tail[at], 22u);
            free(tail);

            return true;
        }
    }

    free(tail);

    return false;
}


/*
d_internal_read_directory
  Reads every directory record into an entry.
*/
static bool
d_internal_read_directory(
    struct d_zip* _zip,
    char*         _problem,
    size_t        _capacity
)
{
    unsigned char end[22];

    // a file with no end record is not an archive at all
    if (!d_internal_find_end(_zip, end))
    {
        return d_internal_problem(_problem, _capacity, "not a ZIP archive",
                                  _zip->path);
    }

    const unsigned      entries = d_internal_le16(&end[10]);
    const unsigned long size    = d_internal_le32(&end[12]);
    const unsigned long start   = d_internal_le32(&end[16]);

    // a split archive, or one whose records overflowed into ZIP64, is not
    // read rather than misread
    if ( (d_internal_le16(&end[4]) != 0u)          ||
         (d_internal_le16(&end[6]) != 0u)          ||
         (entries == 0xFFFFu)                      ||
         (size == 0xFFFFFFFFul)                    ||
         (start == 0xFFFFFFFFul) )
    {
        return d_internal_problem(_problem, _capacity,
                                  "ZIP64 and split archives are not read",
                                  _zip->path);
    }

    unsigned char* const directory = malloc((size_t)size + 1u);

    // a directory that cannot be held or read leaves nothing to list
    if ( (!directory) ||
         (!d_internal_read_at(_zip, start, directory, (size_t)size)) )
    {
        free(directory);

        return d_internal_problem(_problem, _capacity,
                                  "the archive's directory cannot be read",
                                  _zip->path);
    }

    size_t at = 0;
    bool   ok = true;

    for (unsigned index = 0; (ok) && (index < entries); ++index)
    {
        // a record that runs past the directory is a damaged archive
        if ( ((at + 46u) > size)                                      ||
             (d_internal_le32(&directory[at]) != D_INTERNAL_ZIP_CENTRAL) )
        {
            ok = false;
            break;
        }

        const unsigned char* const record   = &directory[at];
        const size_t               name_len = d_internal_le16(&record[28]);
        const size_t               span     = 46u + name_len +
                                              d_internal_le16(&record[30]) +
                                              d_internal_le16(&record[32]);

        // a name or extra field running past the directory is damage too
        if ( ((at + span) > size) ||
             (!d_internal_grow_entries(_zip)) )
        {
            ok = false;
            break;
        }

        struct d_internal_zip_entry* const entry = &_zip->entries[_zip->count];

        memset(entry, 0, sizeof(*entry));
        entry->version_made        = d_internal_le16(&record[4]);
        entry->version_needed      = d_internal_le16(&record[6]);
        entry->flags               = d_internal_le16(&record[8]);
        entry->method              = d_internal_le16(&record[10]);
        entry->time                = d_internal_le16(&record[12]);
        entry->date                = d_internal_le16(&record[14]);
        entry->crc                 = d_internal_le32(&record[16]);
        entry->compressed          = d_internal_le32(&record[20]);
        entry->size                = d_internal_le32(&record[24]);
        entry->internal_attributes = d_internal_le16(&record[36]);
        entry->external_attributes = d_internal_le32(&record[38]);
        entry->offset              = d_internal_le32(&record[42]);
        entry->name                = d_internal_copy_text(
                                         (const char*)&record[46], name_len);

        // a name that cannot be held is an entry that cannot be kept
        if (!entry->name)
        {
            ok = false;
            break;
        }

        _zip->count++;
        at += span;
    }

    free(directory);

    return ok ? true
              : d_internal_problem(_problem, _capacity,
                                   "the archive's directory is damaged",
                                   _zip->path);
}


//==============================================================================
// 6.  LIFECYCLE
//==============================================================================


/*
d_zip_open
  The file's size is taken once, by seeking to its end, so every offset the
directory gives can be checked against it before it is used.
*/
struct d_zip*
d_zip_open(
    const char* _path,
    char*       _problem,
    size_t      _capacity
)
{
    // parameter validation first
    if (!_path)
    {
        (void)d_internal_problem(_problem, _capacity, "no archive named",
                                 NULL);

        return NULL;
    }

    struct d_zip* const zip = calloc(1u, sizeof(*zip));

    // report the failure rather than opening half an archive
    if (!zip)
    {
        (void)d_internal_problem(_problem, _capacity, "out of memory", NULL);

        return NULL;
    }

    zip->path = d_internal_copy_text(_path, strlen(_path));
    zip->file = fopen(_path, "rb");

    // a file that will not open, or whose size cannot be learned, is refused
    if ( (!zip->path)                               ||
         (!zip->file)                               ||
         (fseek(zip->file, 0, SEEK_END) != 0)       ||
         ((zip->file_size = ftell(zip->file)) < 0) )
    {
        (void)d_internal_problem(_problem, _capacity, "cannot read", _path);
        d_zip_free(zip);

        return NULL;
    }

    // a directory that cannot be read leaves no archive
    if (!d_internal_read_directory(zip, _problem, _capacity))
    {
        d_zip_free(zip);

        return NULL;
    }

    return zip;
}


/*
d_zip_new
  An archive with no file behind it: every entry it will hold is added.
*/
struct d_zip*
d_zip_new(void)
{
    return calloc(1u, sizeof(struct d_zip));
}


/*
d_zip_free
  Every entry owns its name and any replacement; the file is closed last.
*/
void
d_zip_free(
    struct d_zip* _zip
)
{
    if (_zip)
    {
        for (size_t at = 0; at < _zip->count; ++at)
        {
            free(_zip->entries[at].name);
            free(_zip->entries[at].bytes);
        }

        // an archive opened from a file closes it
        if (_zip->file)
        {
            (void)fclose(_zip->file);
        }

        free(_zip->entries);
        free(_zip->path);
        free(_zip);
    }

    return;
}


//==============================================================================
// 7.  ENTRIES
//==============================================================================


/*
d_zip_count
  The entries the directory listed, and those added since.
*/
size_t
d_zip_count(
    const struct d_zip* _zip
)
{
    return (_zip) ? _zip->count : 0u;
}


/*
d_zip_name
  An index past the entries names nothing.
*/
const char*
d_zip_name(
    const struct d_zip* _zip,
    size_t              _index
)
{
    return ( (_zip) &&
             (_index < _zip->count) ) ? _zip->entries[_index].name
                                      : NULL;
}


/*
d_zip_find
  A linear search: a package holds hundreds of entries, not millions, and a
lookup is made once per slide.
*/
size_t
d_zip_find(
    const struct d_zip* _zip,
    const char*         _name
)
{
    // parameter validation first
    if ( (!_zip) ||
         (!_name) )
    {
        return D_ZIP_NONE;
    }

    for (size_t at = 0; at < _zip->count; ++at)
    {
        // names are compared exactly, as ZIP stores them
        if (strcmp(_zip->entries[at].name, _name) == 0)
        {
            return at;
        }
    }

    return D_ZIP_NONE;
}


/*
d_internal_data_offset
  Finds where an entry's data begins: after its local header, whose name and
extra field may differ in length from the directory's.
*/
static bool
d_internal_data_offset(
    struct d_zip*                      _zip,
    const struct d_internal_zip_entry* _entry,
    unsigned long*                     _out_offset
)
{
    unsigned char local[30];

    // a local header that cannot be read, or is not one, is damage
    if ( (!d_internal_read_at(_zip, _entry->offset, local, sizeof(local))) ||
         (d_internal_le32(local) != D_INTERNAL_ZIP_LOCAL) )
    {
        return false;
    }

    *_out_offset = _entry->offset + 30ul +
                   (unsigned long)d_internal_le16(&local[26]) +
                   (unsigned long)d_internal_le16(&local[28]);

    return true;
}


/*
d_zip_read
  The checksum is verified after inflating, so a damaged stream that happened
to decode to the right length is still caught.
*/
bool
d_zip_read(
    struct d_zip*   _zip,
    size_t          _index,
    unsigned char** _out_bytes,
    size_t*         _out_length,
    char*           _problem,
    size_t          _capacity
)
{
    // parameter validation first
    if ( (!_zip)                  ||
         (_index >= _zip->count)  ||
         (!_out_bytes)            ||
         (!_out_length) )
    {
        return d_internal_problem(_problem, _capacity, "no such entry", NULL);
    }

    *_out_bytes  = NULL;
    *_out_length = 0;

    const struct d_internal_zip_entry* const entry = &_zip->entries[_index];

    // a replaced or added entry is read from the bytes it now holds
    if ( (entry->bytes) ||
         (!_zip->file) )
    {
        unsigned char* const copy = malloc(entry->length + 1u);

        // report the failure rather than reading nothing
        if (!copy)
        {
            return d_internal_problem(_problem, _capacity, "out of memory",
                                      entry->name);
        }

        memcpy(copy, entry->bytes ? entry->bytes : (unsigned char*)"",
               entry->length);
        copy[entry->length] = '\0';
        *_out_bytes         = copy;
        *_out_length        = entry->length;

        return true;
    }

    // an encrypted entry, or one compressed some other way, is not read
    if ( ((entry->flags & 0x0001u) != 0u) ||
         ( (entry->method != 0u) &&
           (entry->method != 8u) ) )
    {
        return d_internal_problem(_problem, _capacity,
                                  "entry is encrypted or uses a compression "
                                  "method other than store and DEFLATE",
                                  entry->name);
    }

    unsigned long offset = 0;

    unsigned char* const packed = malloc((size_t)entry->compressed + 1u);
    unsigned char* const bytes  = (entry->method == 0u)
                                ? packed
                                : malloc((size_t)entry->size + 1u);
    bool                 ok     = ( (packed) &&
                                    (bytes) &&
                                    (d_internal_data_offset(_zip,
                                                            entry,
                                                            &offset)) &&
                                    (d_internal_read_at(
                                         _zip,
                                         offset,
                                         packed,
                                         (size_t)entry->compressed)) );

    // a stored entry's bytes are its data; a deflated one's are inflated
    if ( (ok) &&
         (entry->method == 8u) )
    {
        ok = d_internal_inflate_all(packed,
                                    (size_t)entry->compressed,
                                    bytes,
                                    (size_t)entry->size);
    }

    // stored data longer or shorter than the entry is damage as well
    if ( (ok) &&
         (entry->method == 0u) &&
         (entry->compressed != entry->size) )
    {
        ok = false;
    }

    // the checksum is the last word on whether the bytes are the entry's
    if ( (ok) &&
         (d_zip_crc32(0ul, bytes, (size_t)entry->size) != entry->crc) )
    {
        ok = false;
    }

    // the packed copy is released unless it is the entry's bytes
    if (packed != bytes)
    {
        free(packed);
    }

    // a read that failed anywhere releases what it held
    if (!ok)
    {
        free(bytes);

        return d_internal_problem(_problem, _capacity,
                                  "entry is damaged or cannot be read",
                                  entry->name);
    }

    bytes[entry->size] = '\0';
    *_out_bytes        = bytes;
    *_out_length       = (size_t)entry->size;

    return true;
}


//==============================================================================
// 8.  CHANGES
//==============================================================================


/*
d_internal_hold
  Copies bytes into an entry as its replacement, releasing any it had.
*/
static bool
d_internal_hold(
    struct d_internal_zip_entry* _entry,
    const unsigned char*         _bytes,
    size_t                       _length
)
{
    // a replacement past what a ZIP entry without ZIP64 can hold is refused
    if (_length > 0xFFFFFFFEul)
    {
        return false;
    }

    unsigned char* const copy = malloc(_length + 1u);

    // report the failure rather than keeping half the replacement
    if (!copy)
    {
        return false;
    }

    // an empty replacement has no bytes to copy
    if (_length > 0)
    {
        memcpy(copy, _bytes, _length);
    }

    free(_entry->bytes);
    _entry->bytes      = copy;
    _entry->length     = _length;
    _entry->method     = 0u;
    _entry->flags     &= D_INTERNAL_ZIP_UTF8;
    _entry->crc        = d_zip_crc32(0ul, copy, _length);
    _entry->compressed = (unsigned long)_length;
    _entry->size       = (unsigned long)_length;

    return true;
}


/*
d_zip_replace
  The entry keeps its name, its place and its timestamps; only its contents
and what describes them change.
*/
bool
d_zip_replace(
    struct d_zip*        _zip,
    size_t               _index,
    const unsigned char* _bytes,
    size_t               _length
)
{
    // parameter validation first
    if ( (!_zip)                                ||
         (_index >= _zip->count)                ||
         ( (!_bytes) && (_length > 0) ) )
    {
        return false;
    }

    return d_internal_hold(&_zip->entries[_index], _bytes, _length);
}


/*
d_zip_add
  An added entry is stamped with the DOS epoch, 1980-01-01, rather than the
clock, so saving one archive twice gives the same bytes.
*/
bool
d_zip_add(
    struct d_zip*        _zip,
    const char*          _name,
    const unsigned char* _bytes,
    size_t               _length
)
{
    // parameter validation first
    if ( (!_zip)                                ||
         (!_name)                               ||
         (_name[0] == '\0')                     ||
         ( (!_bytes) && (_length > 0) )         ||
         (d_zip_find(_zip, _name) != D_ZIP_NONE) )
    {
        return false;
    }

    // abandon the addition when the table cannot grow
    if (!d_internal_grow_entries(_zip))
    {
        return false;
    }

    struct d_internal_zip_entry* const entry = &_zip->entries[_zip->count];

    memset(entry, 0, sizeof(*entry));
    entry->version_made   = 20u;
    entry->version_needed = 10u;
    entry->flags          = D_INTERNAL_ZIP_UTF8;
    entry->date           = (0u << 9) | (1u << 5) | 1u;
    entry->name           = d_internal_copy_text(_name, strlen(_name));

    // an entry without its name or its bytes is not added at all
    if ( (!entry->name) ||
         (!d_internal_hold(entry, _bytes, _length)) )
    {
        free(entry->name);
        free(entry->bytes);

        return false;
    }

    _zip->count++;

    return true;
}


//==============================================================================
// 9.  SAVING
//==============================================================================


/*
d_internal_copy_data
  Copies an unchanged entry's stored bytes from the archive to the output in
pieces, so a large media file never needs to be held whole.
*/
static bool
d_internal_copy_data(
    struct d_zip* _zip,
    unsigned long _offset,
    unsigned long _length,
    FILE*         _out
)
{
    unsigned char piece[65536];

    // abandon the copy when the data cannot be reached
    if ( (_offset > (unsigned long)LONG_MAX)                         ||
         (fseek(_zip->file, (long)_offset, SEEK_SET) != 0) )
    {
        return false;
    }

    while (_length > 0)
    {
        const size_t want = (_length < sizeof(piece)) ? (size_t)_length
                                                      : sizeof(piece);

        // a short read or write is a failed copy
        if ( (fread(piece, 1u, want, _zip->file) != want) ||
             (fwrite(piece, 1u, want, _out) != want) )
        {
            return false;
        }

        _length -= (unsigned long)want;
    }

    return true;
}


/*
d_internal_write_entry
  Writes one entry's local header and data, recording where it begins.
*/
static bool
d_internal_write_entry(
    struct d_zip*                _zip,
    struct d_internal_zip_entry* _entry,
    FILE*                        _out,
    unsigned long*               _out_offset
)
{
    const long here = ftell(_out);

    // an output past what a ZIP without ZIP64 can address is refused
    if ( (here < 0) ||
         ((unsigned long)here > 0xFFFFFFFEul) )
    {
        return false;
    }

    unsigned long source = 0;

    // an unchanged entry's data is found before anything is written
    if ( (!_entry->bytes) &&
         (!d_internal_data_offset(_zip, _entry, &source)) )
    {
        return false;
    }

    const size_t  name_length = strlen(_entry->name);
    unsigned char local[30];

    d_internal_put32(&local[0], D_INTERNAL_ZIP_LOCAL);
    d_internal_put16(&local[4], _entry->version_needed);
    d_internal_put16(&local[6], _entry->flags & ~0x0008u);
    d_internal_put16(&local[8], _entry->method);
    d_internal_put16(&local[10], _entry->time);
    d_internal_put16(&local[12], _entry->date);
    d_internal_put32(&local[14], _entry->crc);
    d_internal_put32(&local[18], _entry->compressed);
    d_internal_put32(&local[22], _entry->size);
    d_internal_put16(&local[26], (unsigned long)name_length);
    d_internal_put16(&local[28], 0ul);

    *_out_offset = (unsigned long)here;

    // the header and name, then the data as it is held or as it was stored
    if ( (fwrite(local, 1u, sizeof(local), _out) != sizeof(local)) ||
         (fwrite(_entry->name, 1u, name_length, _out) != name_length) )
    {
        return false;
    }

    return (_entry->bytes)
         ? (fwrite(_entry->bytes, 1u, _entry->length, _out) == _entry->length)
         : d_internal_copy_data(_zip, source, _entry->compressed, _out);
}


/*
d_internal_write_directory
  Writes the directory records and the end record after the entries.
*/
static bool
d_internal_write_directory(
    const struct d_zip*  _zip,
    const unsigned long* _offsets,
    FILE*                _out
)
{
    const long start = ftell(_out);

    // a directory past what a ZIP without ZIP64 can address is refused
    if ( (start < 0) ||
         ((unsigned long)start > 0xFFFFFFFEul) )
    {
        return false;
    }

    for (size_t at = 0; at < _zip->count; ++at)
    {
        const struct d_internal_zip_entry* const entry = &_zip->entries[at];
        const size_t  name_length = strlen(entry->name);
        unsigned char record[46];

        d_internal_put32(&record[0], D_INTERNAL_ZIP_CENTRAL);
        d_internal_put16(&record[4], entry->version_made);
        d_internal_put16(&record[6], entry->version_needed);
        d_internal_put16(&record[8], entry->flags & ~0x0008u);
        d_internal_put16(&record[10], entry->method);
        d_internal_put16(&record[12], entry->time);
        d_internal_put16(&record[14], entry->date);
        d_internal_put32(&record[16], entry->crc);
        d_internal_put32(&record[20], entry->compressed);
        d_internal_put32(&record[24], entry->size);
        d_internal_put16(&record[28], (unsigned long)name_length);
        d_internal_put16(&record[30], 0ul);
        d_internal_put16(&record[32], 0ul);
        d_internal_put16(&record[34], 0ul);
        d_internal_put16(&record[36], entry->internal_attributes);
        d_internal_put32(&record[38], entry->external_attributes);
        d_internal_put32(&record[42], _offsets[at]);

        // a short write of a record is a failed save
        if ( (fwrite(record, 1u, sizeof(record), _out) != sizeof(record)) ||
             (fwrite(entry->name, 1u, name_length, _out) != name_length) )
        {
            return false;
        }
    }

    const long end = ftell(_out);

    // a directory that cannot be measured cannot be recorded
    if (end < start)
    {
        return false;
    }

    unsigned char record[22];

    memset(record, 0, sizeof(record));
    d_internal_put32(&record[0], D_INTERNAL_ZIP_END);
    d_internal_put16(&record[8], (unsigned long)_zip->count);
    d_internal_put16(&record[10], (unsigned long)_zip->count);
    d_internal_put32(&record[12], (unsigned long)(end - start));
    d_internal_put32(&record[16], (unsigned long)start);

    return (fwrite(record, 1u, sizeof(record), _out) == sizeof(record));
}


/*
d_zip_save
  Written to `<path>.part` and renamed into place.  rename replaces an
existing file on POSIX and refuses to on Windows, so on refusal the old file
is removed and the rename tried once more.
*/
bool
d_zip_save(
    struct d_zip* _zip,
    const char*   _path,
    char*         _problem,
    size_t        _capacity
)
{
    // parameter validation first
    if ( (!_zip) ||
         (!_path) )
    {
        return d_internal_problem(_problem, _capacity, "nothing to save",
                                  NULL);
    }

    // the archive's own file is not overwritten
    if ( (_zip->path) &&
         (strcmp(_zip->path, _path) == 0) )
    {
        return d_internal_problem(_problem, _capacity,
                                  "will not save over the archive itself",
                                  _path);
    }

    // more entries than an end record counts need ZIP64
    if (_zip->count > 0xFFFEu)
    {
        return d_internal_problem(_problem, _capacity,
                                  "too many entries without ZIP64", _path);
    }

    const size_t length  = strlen(_path);
    char* const  part    = malloc(length + sizeof(".part"));
    unsigned long* const offsets = calloc(_zip->count + 1u, sizeof(*offsets));

    // abandon the save when its bookkeeping cannot be held
    if ( (!part) ||
         (!offsets) )
    {
        free(part);
        free(offsets);

        return d_internal_problem(_problem, _capacity, "out of memory", NULL);
    }

    memcpy(part, _path, length);
    memcpy(&part[length], ".part", sizeof(".part"));

    FILE* const out = fopen(part, "wb");
    bool        ok  = (out != NULL);

    // each entry, then the directory that lists them
    for (size_t at = 0; (ok) && (at < _zip->count); ++at)
    {
        ok = d_internal_write_entry(_zip, &_zip->entries[at], out,
                                    &offsets[at]);
    }

    ok = (ok) && d_internal_write_directory(_zip, offsets, out);

    // a stream that fails to close has not written what it held
    if ( (out) &&
         (fclose(out) != 0) )
    {
        ok = false;
    }

    // the whole archive replaces the old file, or nothing does
    if ( (ok) &&
         (rename(part, _path) != 0) )
    {
        (void)remove(_path);
        ok = (rename(part, _path) == 0);
    }

    // a failed save leaves no partial file behind
    if (!ok)
    {
        (void)remove(part);
        (void)d_internal_problem(_problem, _capacity, "cannot write", _path);
    }

    free(part);
    free(offsets);

    return ok;
}
