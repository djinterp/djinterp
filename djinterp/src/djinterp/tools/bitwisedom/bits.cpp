/*******************************************************************************
* bitwisedom [bitwisedom]                                               bits.cpp
*
* Bits within a width: the definitions behind bits.hpp.
*   All arithmetic is unsigned, so a 64-bit width never shifts by 64 or
* overflows a signed integer; a signed result is formed only at the end, from
* a value already known to fit.
*   A wide value keeps its words without a zero word at the top, so two equal
* sets of bits are equal vectors, and a narrow value is one word or none.
* Its arithmetic goes a word at a time with the carry beside it; its product
* with an integer builds each 128-bit partial product from 32-bit halves,
* which every compiler has.
*
* path:      /src/djinterp/tools/bitwisedom/bits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/bits.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::max, std::min
#include <cctype>     // std::isspace, std::tolower
#include <cstddef>    // std::size_t
#include <string>     // std::string
// re_std
#include "re_std/bit/countl_zero.hpp"  // re_std::countl_zero
#include "re_std/bit/countr_zero.hpp"  // re_std::countr_zero
#include "re_std/bit/popcount.hpp"     // re_std::popcount


namespace bitwisedom
{

namespace
{

// clamp_width
//   a width held to [0, INTEGER_WIDTH]: the functions on u64.
int
clamp_width(
    int _width
)
{
    if (_width < 0)
    {
        return 0;
    }

    return (_width > INTEGER_WIDTH) ? INTEGER_WIDTH
                                    : _width;
}

// clamp_wide
//   a width held to [0, MAX_WIDTH]: the functions on wide values.
int
clamp_wide(
    int _width
)
{
    return std::max(0, std::min(_width, MAX_WIDTH));
}

// WORD
//   constant: the bits in one of a wide value's words.
const int WORD = 64;

/*
multiply_word
  The 128-bit product of two words, as a high and a low word, from 32-bit
halves: a * b = (ah * 2^32 + al) * (bh * 2^32 + bl).
*/
void
multiply_word(
    u64  _a,
    u64  _b,
    u64& _high,
    u64& _low
)
{
    const u64 al = _a & 0xFFFFFFFFu;
    const u64 ah = _a >> 32;
    const u64 bl = _b & 0xFFFFFFFFu;
    const u64 bh = _b >> 32;

    const u64 ll = al * bl;
    const u64 lh = al * bh;
    const u64 hl = ah * bl;
    const u64 hh = ah * bh;

    // the middle terms, with what carries out of the low word
    const u64 middle = (ll >> 32) + (lh & 0xFFFFFFFFu) + (hl & 0xFFFFFFFFu);

    _low  = (middle << 32) | (ll & 0xFFFFFFFFu);
    _high = hh + (lh >> 32) + (hl >> 32) + (middle >> 32);

    return;
}

// digit_value
//   the value of one digit character in any base up to 16, or -1.
int
digit_value(
    char _character
)
{
    if ( (_character >= '0') &&
         (_character <= '9') )
    {
        return _character - '0';
    }

    const char lower =
        static_cast<char>(std::tolower(static_cast<unsigned char>(_character)));

    if ( (lower >= 'a') &&
         (lower <= 'f') )
    {
        return lower - 'a' + 10;
    }

    return -1;
}

// is_suffix
//   whether text is a C integer suffix this reader accepts and ignores.
bool
is_suffix(
    std::string_view _text
)
{
    std::string lower;

    for (char each : _text)
    {
        lower += static_cast<char>(
            std::tolower(static_cast<unsigned char>(each)));
    }

    return ( (lower == "u")   ||
             (lower == "l")   ||
             (lower == "ul")  ||
             (lower == "lu")  ||
             (lower == "ll")  ||
             (lower == "ull") ||
             (lower == "llu") );
}

}  // anonymous namespace


// ------------------------------------------------------------ masks and runs

u64
width_mask(
    int _width
)
{
    const int width = clamp_width(_width);

    return (width >= INTEGER_WIDTH) ? ~u64(0)
                                    : ((u64(1) << width) - 1u);
}

u64
range_mask(
    int _lsb,
    int _width
)
{
    // a range starting past the top holds no bits
    if ( (_lsb < 0)              ||
         (_lsb >= INTEGER_WIDTH) ||
         (_width <= 0) )
    {
        return 0;
    }

    return width_mask(_width) << _lsb;
}

int
bit_count(
    u64 _value
)
{
    return re_std::popcount(_value);
}

int
lowest_set(
    u64 _value
)
{
    return (_value == 0) ? -1
                         : re_std::countr_zero(_value);
}

int
highest_set(
    u64 _value
)
{
    return (_value == 0) ? -1
                         : (INTEGER_WIDTH - 1 - re_std::countl_zero(_value));
}

/*
is_run
  A run of ones shifted down to bit zero is one less than a power of two, so
adding one to it leaves a single bit; anything else leaves more.
*/
bool
is_run(
    u64  _value,
    int& _lsb,
    int& _width
)
{
    if (_value == 0)
    {
        return false;
    }

    const int low     = lowest_set(_value);
    const u64 shifted = _value >> low;

    // all ones to the top is a run that the +1 test would overflow
    if (shifted == ~u64(0) >> low)
    {
        _lsb   = low;
        _width = INTEGER_WIDTH - low;

        return true;
    }

    if ((shifted & (shifted + 1u)) != 0)
    {
        return false;
    }

    _lsb   = low;
    _width = bit_count(_value);

    return true;
}

// ----------------------------------------------------------------- movement

u64
rotate_left(
    u64 _value,
    int _count,
    int _width
)
{
    const int width = clamp_width(_width);

    // nothing to rotate in a width of nothing
    if (width == 0)
    {
        return 0;
    }

    const u64 value = _value & width_mask(width);
    const int count = ((_count % width) + width) % width;

    if (count == 0)
    {
        return value;
    }

    return ( (value << count) |
             (value >> (width - count)) ) & width_mask(width);
}

u64
rotate_right(
    u64 _value,
    int _count,
    int _width
)
{
    const int width = clamp_width(_width);

    if (width == 0)
    {
        return 0;
    }

    return rotate_left(_value,
                       width - (((_count % width) + width) % width),
                       width);
}

u64
reverse_bits(
    u64 _value,
    int _width
)
{
    const int width  = clamp_width(_width);
    u64       result = 0;

    // bit i moves to bit width-1-i
    for (int i = 0; i < width; ++i)
    {
        if (((_value >> i) & 1u) != 0)
        {
            result |= u64(1) << (width - 1 - i);
        }
    }

    return result;
}

/*
reverse_bytes
  A width that is not a whole number of bytes is treated as the bytes that
cover it, with the top byte's missing bits as zeros; the result is cut back
to the width, so the bits that were padding fall away.
*/
u64
reverse_bytes(
    u64 _value,
    int _width
)
{
    const int width  = clamp_width(_width);
    const int bytes  = (width + 7) / 8;
    const u64 value  = _value & width_mask(width);
    u64       result = 0;

    for (int i = 0; i < bytes; ++i)
    {
        const u64 byte = (value >> (8 * i)) & 0xFFu;

        result |= byte << (8 * (bytes - 1 - i));
    }

    return result & width_mask(width);
}

i64
sign_extend(
    u64 _value,
    int _width
)
{
    const int width = clamp_width(_width);

    // a width of nothing holds zero
    if (width == 0)
    {
        return 0;
    }

    const u64 value = _value & width_mask(width);
    const u64 sign  = u64(1) << (width - 1);

    // the sign bit set: the value less 2^width, done without overflow
    if ((value & sign) != 0)
    {
        const u64 magnitude = (~value & width_mask(width)) + 1u;

        return (magnitude == (u64(1) << 63))
                   ? static_cast<i64>(-9223372036854775807LL - 1)
                   : -static_cast<i64>(magnitude);
    }

    return static_cast<i64>(value);
}


// --------------------------------------------------------------- wide values

wide::wide(
    u64 _low
)
{
    if (_low != 0)
    {
        m_words.push_back(_low);
    }
}

wide
wide::bit(
    int _index
)
{
    wide out;
    out.set(_index, true);

    return out;
}

wide
wide::run(
    int _lsb,
    int _width
)
{
    wide      out;
    const int low  = std::max(0, _lsb);
    const int high = clamp_wide(_lsb + _width);

    if (high <= low)
    {
        return out;
    }

    out.m_words.assign(static_cast<std::size_t>((high + WORD - 1) / WORD), 0);

    // whole words at once, and the ends a bit at a time
    for (int i = low; i < high; )
    {
        if ( ((i % WORD) == 0) &&
             (i + WORD <= high) )
        {
            out.m_words[static_cast<std::size_t>(i / WORD)] = ~u64(0);
            i += WORD;
        }
        else
        {
            out.m_words[static_cast<std::size_t>(i / WORD)] |=
                u64(1) << (i % WORD);
            ++i;
        }
    }

    out.normalize();

    return out;
}

bool
wide::test(
    int _index
) const
{
    if (_index < 0)
    {
        return false;
    }

    return ((word(_index / WORD) >> (_index % WORD)) & 1u) != 0;
}

void
wide::set(
    int  _index,
    bool _on
)
{
    if ( (_index < 0) ||
         (_index >= MAX_WIDTH) )
    {
        return;
    }

    const std::size_t at  = static_cast<std::size_t>(_index / WORD);
    const u64         one = u64(1) << (_index % WORD);

    if (_on)
    {
        if (at >= m_words.size())
        {
            m_words.resize(at + 1, 0);
        }

        m_words[at] |= one;
    }
    else if (at < m_words.size())
    {
        m_words[at] &= ~one;
        normalize();
    }

    return;
}

u64
wide::word(
    int _index
) const
{
    return ( (_index >= 0) &&
             (static_cast<std::size_t>(_index) < m_words.size()) )
               ? m_words[static_cast<std::size_t>(_index)]
               : 0;
}

u64
wide::low() const
{
    return word(0);
}

int
wide::words() const
{
    return static_cast<int>(m_words.size());
}

u64
wide::field(
    int _lsb,
    int _width
) const
{
    const int width = clamp_width(_width);

    if ( (_lsb < 0) ||
         (width == 0) )
    {
        return 0;
    }

    const int at    = _lsb / WORD;
    const int shift = _lsb % WORD;
    u64       bits  = word(at) >> shift;

    // the field runs on into the next word
    if ( (shift > 0) &&
         (width > WORD - shift) )
    {
        bits |= word(at + 1) << (WORD - shift);
    }

    return bits & width_mask(width);
}

void
wide::put(
    int _lsb,
    int _width,
    u64 _bits
)
{
    const int width = clamp_width(_width);

    if ( (_lsb < 0) ||
         (width == 0) )
    {
        return;
    }

    // the field, cleared and written a word at a time: its part in the
    // word it starts in, and what runs on into the next
    const int at    = _lsb / WORD;
    const int shift = _lsb % WORD;
    const u64 mask  = width_mask(width);
    const u64 bits  = _bits & mask;

    if ((at + 1) * WORD > MAX_WIDTH)
    {
        return;
    }

    const std::size_t need = static_cast<std::size_t>(at) +
                             ( (shift > 0) &&
                               (width > WORD - shift) &&
                               ((at + 2) * WORD <= MAX_WIDTH) ? 2u : 1u );

    if (m_words.size() < need)
    {
        m_words.resize(need, 0);
    }

    m_words[static_cast<std::size_t>(at)] =
        (m_words[static_cast<std::size_t>(at)] & ~(mask << shift)) |
        (bits << shift);

    if (need == static_cast<std::size_t>(at) + 2u)
    {
        const std::size_t next = static_cast<std::size_t>(at) + 1u;

        m_words[next] = (m_words[next] & ~(mask >> (WORD - shift))) |
                        (bits >> (WORD - shift));
    }

    normalize();

    return;
}

wide
wide::slice(
    int _lsb,
    int _width
) const
{
    wide out;

    for (int i = 0; i < _width; i += WORD)
    {
        const int part = std::min(WORD, _width - i);
        const u64 bits = field(_lsb + i, part);

        if (bits != 0)
        {
            out.put(i, part, bits);
        }
    }

    return out;
}

bool
wide::any() const
{
    return !m_words.empty();
}

int
wide::count() const
{
    int total = 0;

    for (u64 each : m_words)
    {
        total += re_std::popcount(each);
    }

    return total;
}

int
wide::lowest() const
{
    for (std::size_t i = 0; i < m_words.size(); ++i)
    {
        if (m_words[i] != 0)
        {
            return static_cast<int>(i) * WORD +
                   re_std::countr_zero(m_words[i]);
        }
    }

    return -1;
}

int
wide::highest() const
{
    if (m_words.empty())
    {
        return -1;
    }

    return (static_cast<int>(m_words.size()) - 1) * WORD +
           highest_set(m_words.back());
}

int
wide::next(
    int _from
) const
{
    int at = std::max(0, _from);

    while (at / WORD < static_cast<int>(m_words.size()))
    {
        const u64 rest = word(at / WORD) >> (at % WORD);

        if (rest != 0)
        {
            return at + re_std::countr_zero(rest);
        }

        at = (at / WORD + 1) * WORD;
    }

    return -1;
}

bool
wide::fits(
    int _width
) const
{
    return (highest() < _width);
}

void
wide::trim(
    int _width
)
{
    const int width = clamp_wide(_width);
    const int keep  = (width + WORD - 1) / WORD;

    if (static_cast<int>(m_words.size()) > keep)
    {
        m_words.resize(static_cast<std::size_t>(keep));
    }

    if ( ((width % WORD) != 0)                            &&
         (static_cast<int>(m_words.size()) == keep)       &&
         (keep > 0) )
    {
        m_words.back() &= width_mask(width % WORD);
    }

    normalize();

    return;
}

wide&
wide::operator|=(
    const wide& _other
)
{
    if (_other.m_words.size() > m_words.size())
    {
        m_words.resize(_other.m_words.size(), 0);
    }

    for (std::size_t i = 0; i < _other.m_words.size(); ++i)
    {
        m_words[i] |= _other.m_words[i];
    }

    return *this;
}

wide&
wide::operator&=(
    const wide& _other
)
{
    if (m_words.size() > _other.m_words.size())
    {
        m_words.resize(_other.m_words.size());
    }

    for (std::size_t i = 0; i < m_words.size(); ++i)
    {
        m_words[i] &= _other.m_words[i];
    }

    normalize();

    return *this;
}

wide&
wide::operator^=(
    const wide& _other
)
{
    if (_other.m_words.size() > m_words.size())
    {
        m_words.resize(_other.m_words.size(), 0);
    }

    for (std::size_t i = 0; i < _other.m_words.size(); ++i)
    {
        m_words[i] ^= _other.m_words[i];
    }

    normalize();

    return *this;
}

bool
wide::operator==(
    const wide& _other
) const
{
    return (m_words == _other.m_words);
}

bool
wide::operator!=(
    const wide& _other
) const
{
    return !(*this == _other);
}

void
wide::normalize()
{
    while ( (!m_words.empty()) &&
            (m_words.back() == 0) )
    {
        m_words.pop_back();
    }

    return;
}

wide
operator|(
    wide        _a,
    const wide& _b
)
{
    _a |= _b;

    return _a;
}

wide
operator&(
    wide        _a,
    const wide& _b
)
{
    _a &= _b;

    return _a;
}

wide
operator^(
    wide        _a,
    const wide& _b
)
{
    _a ^= _b;

    return _a;
}

wide
complement(
    const wide& _value,
    int         _width
)
{
    return wide::run(0, _width) ^ _value;
}

/*
shift_left
  Each word moves up by the whole words in the count, and its bits by the
rest: the low part of it into the word it lands in, the high part into the
one above.
*/
wide
shift_left(
    const wide& _value,
    int         _count,
    int         _width
)
{
    if (_count < 0)
    {
        return shift_right(_value, -_count);
    }

    wide      out;
    const int whole = _count / WORD;
    const int bits  = _count % WORD;

    for (int i = 0; i < _value.words(); ++i)
    {
        const u64 each = _value.word(i);
        const int to   = i + whole;

        if (each == 0)
        {
            continue;
        }

        if (to * WORD < MAX_WIDTH)
        {
            out.put(to * WORD,
                    WORD,
                    out.word(to) | ((bits == 0) ? each : (each << bits)));
        }

        if ( (bits != 0) &&
             ((to + 1) * WORD < MAX_WIDTH) )
        {
            out.put((to + 1) * WORD,
                    WORD,
                    out.word(to + 1) | (each >> (WORD - bits)));
        }
    }

    out.trim(_width);

    return out;
}

wide
shift_right(
    const wide& _value,
    int         _count
)
{
    if (_count <= 0)
    {
        return (_count == 0) ? _value
                             : shift_left(_value, -_count, MAX_WIDTH);
    }

    wide      out;
    const int top = _value.highest();

    for (int i = _count; i <= top; i += WORD)
    {
        const int part = std::min(WORD, top - i + 1);
        const u64 bits = _value.field(i, part);

        if (bits != 0)
        {
            out.put(i - _count, part, bits);
        }
    }

    return out;
}

wide
rotate_left(
    const wide& _value,
    int         _count,
    int         _width
)
{
    const int width = clamp_wide(_width);

    if (width == 0)
    {
        return wide();
    }

    // a count of either sign, brought into [0, width)
    const int turn  = ((_count % width) + width) % width;
    wide      value = _value;

    value.trim(width);

    return shift_left(value, turn, width) |
           shift_right(value, width - turn);
}

wide
rotate_right(
    const wide& _value,
    int         _count,
    int         _width
)
{
    const int width = clamp_wide(_width);

    if (width == 0)
    {
        return wide();
    }

    return rotate_left(_value,
                       width - (((_count % width) + width) % width),
                       width);
}

wide
reverse_bits(
    const wide& _value,
    int         _width
)
{
    const int width = clamp_wide(_width);
    wide      out;

    for (int i = _value.next(0);
         (i >= 0) && (i < width);
         i = _value.next(i + 1))
    {
        out.set(width - 1 - i, true);
    }

    return out;
}

/*
reverse_bytes
  The bytes of the width, last first, as the u64 version moves them: a
width that is no whole number of bytes is taken as the bytes that hold it,
and the result held to the width again.
*/
wide
reverse_bytes(
    const wide& _value,
    int         _width
)
{
    const int width = clamp_wide(_width);
    const int bytes = (width + 7) / 8;
    wide      value = _value;
    wide      out;

    value.trim(width);

    for (int b = 0; b < bytes; ++b)
    {
        const u64 byte = value.field(b * 8, 8);

        if (byte != 0)
        {
            out.put((bytes - 1 - b) * 8, 8, byte);
        }
    }

    out.trim(width);

    return out;
}

wide
add(
    const wide& _a,
    const wide& _b,
    int         _width
)
{
    const int words = std::max(_a.words(), _b.words()) + 1;
    wide      out;
    u64       carry = 0;

    for (int i = 0; i < words; ++i)
    {
        const u64 a   = _a.word(i);
        const u64 b   = _b.word(i);
        const u64 sum = a + b;
        const u64 all = sum + carry;

        carry = ( (sum < a) || (all < sum) ) ? 1u : 0u;

        if ( (all != 0) &&
             ((i + 1) * WORD <= MAX_WIDTH) )
        {
            out.put(i * WORD, WORD, all);
        }
    }

    out.trim(_width);

    return out;
}

wide
subtract(
    const wide& _a,
    const wide& _b,
    int         _width
)
{
    // a - b is a + ~b + 1, within the width
    return add(add(_a, complement(_b, _width), _width), wide(1), _width);
}

wide
negate(
    const wide& _value,
    int         _width
)
{
    return subtract(wide(), _value, _width);
}

wide
multiply(
    const wide& _value,
    u64         _by,
    int         _width
)
{
    wide out;
    u64  carry = 0;

    for (int i = 0; i <= _value.words(); ++i)
    {
        u64 high = 0;
        u64 low  = 0;

        multiply_word(_value.word(i), _by, high, low);

        const u64 all = low + carry;

        carry = high + ((all < low) ? 1u : 0u);

        if ( (all != 0) &&
             ((i + 1) * WORD <= MAX_WIDTH) )
        {
            out.put(i * WORD, WORD, all);
        }
    }

    out.trim(_width);

    return out;
}

wide
widen_signed(
    i64 _value,
    int _width
)
{
    if (_value >= 0)
    {
        wide out(static_cast<u64>(_value));
        out.trim(_width);

        return out;
    }

    // the low word is the pattern; every bit above it, the sign
    wide out = wide::run(0, _width);

    out.put(0, WORD, static_cast<u64>(_value));
    out.trim(_width);

    return out;
}

bool
is_run(
    const wide& _value,
    int&        _lsb,
    int&        _width
)
{
    if (!_value.any())
    {
        return false;
    }

    const int low  = _value.lowest();
    const int high = _value.highest();

    if (_value.count() != high - low + 1)
    {
        return false;
    }

    _lsb   = low;
    _width = high - low + 1;

    return true;
}


// ---------------------------------------------------------------- encodings

const char*
encoding_name(
    encoding _encoding
)
{
    switch (_encoding)
    {
        case encoding::unsigned_binary: return "unsigned";
        case encoding::twos_complement: return "two's complement";
        case encoding::ones_complement: return "ones' complement";
        case encoding::sign_magnitude:  return "sign-magnitude";
        case encoding::offset_binary:   return "offset binary";
    }

    return "unsigned";
}

i64
default_bias(
    int _width
)
{
    const int width = clamp_width(_width);

    // excess-2^(n-1), the bias offset binary usually means
    if ( (width == 0) ||
         (width > 63) )
    {
        return 0;
    }

    return static_cast<i64>(u64(1) << (width - 1));
}

/*
range_of
  A width of 64 cannot hold its unsigned maximum in an i64, so the unsigned
range there stops at the i64 maximum; every other case is exact.
*/
value_range
range_of(
    encoding _encoding,
    int      _width,
    i64      _bias
)
{
    const int   width = clamp_width(_width);
    value_range range;

    if (width == 0)
    {
        return range;
    }

    const u64 top  = (width >= 64) ? ~u64(0) >> 1
                                   : width_mask(width);
    const u64 half = (width >= 64) ? ~u64(0) >> 1
                                   : width_mask(width - 1);

    switch (_encoding)
    {
        case encoding::unsigned_binary:
            range.low  = 0;
            range.high = static_cast<i64>(top);
            break;

        case encoding::twos_complement:
            range.low  = -static_cast<i64>(half) - 1;
            range.high = static_cast<i64>(half);
            break;

        case encoding::ones_complement:
        case encoding::sign_magnitude:
            range.low  = -static_cast<i64>(half);
            range.high = static_cast<i64>(half);
            break;

        case encoding::offset_binary:
            range.low  = -_bias;
            range.high = static_cast<i64>(top) - _bias;
            break;
    }

    return range;
}

bool
encode(
    encoding _encoding,
    int      _width,
    i64      _bias,
    i64      _value,
    u64&     _bits
)
{
    const int         width = clamp_width(_width);
    const u64         mask  = width_mask(width);
    const value_range range = range_of(_encoding, width, _bias);
    const bool        fits  = ( (_value >= range.low) &&
                                (_value <= range.high) );
    const u64         raw   = static_cast<u64>(_value);
    const u64         size  = (raw >> 63) ? (~raw + 1u)
                                          : raw;

    switch (_encoding)
    {
        case encoding::unsigned_binary:
        case encoding::twos_complement:
            _bits = raw & mask;
            break;

        case encoding::ones_complement:
            _bits = ((raw >> 63) ? ~size : size) & mask;
            break;

        case encoding::sign_magnitude:
            _bits = (width == 0)
                        ? 0
                        : ( (size & width_mask(width - 1)) |
                            ((raw >> 63) ? (u64(1) << (width - 1)) : 0) );
            break;

        case encoding::offset_binary:
            _bits = (raw + static_cast<u64>(_bias)) & mask;
            break;
    }

    return fits;
}

i64
decode(
    encoding _encoding,
    int      _width,
    i64      _bias,
    u64      _bits
)
{
    const int width = clamp_width(_width);
    const u64 bits  = _bits & width_mask(width);

    if (width == 0)
    {
        return 0;
    }

    const u64 sign = u64(1) << (width - 1);

    switch (_encoding)
    {
        case encoding::unsigned_binary:
            return static_cast<i64>(bits);

        case encoding::twos_complement:
            return sign_extend(bits, width);

        case encoding::ones_complement:
            return ((bits & sign) != 0)
                       ? -static_cast<i64>(~bits & width_mask(width))
                       : static_cast<i64>(bits);

        case encoding::sign_magnitude:
            return ((bits & sign) != 0)
                       ? -static_cast<i64>(bits & width_mask(width - 1))
                       : static_cast<i64>(bits);

        case encoding::offset_binary:
            return static_cast<i64>(bits - static_cast<u64>(_bias));
    }

    return static_cast<i64>(bits);
}


// --------------------------------------------------------------- formatting

std::string
format_hex(
    u64  _value,
    int  _digits,
    bool _upper
)
{
    const char* table = _upper ? "0123456789ABCDEF"
                               : "0123456789abcdef";
    std::string out;
    u64         value = _value;

    // at least one digit, and as many as asked
    do
    {
        out.insert(out.begin(), table[value & 0xFu]);
        value >>= 4;
    }
    while (value != 0);

    while (static_cast<int>(out.size()) < _digits)
    {
        out.insert(out.begin(), '0');
    }

    return out;
}

std::string
format_oct(
    u64 _value
)
{
    std::string out;
    u64         value = _value;

    do
    {
        out.insert(out.begin(), static_cast<char>('0' + (value & 7u)));
        value >>= 3;
    }
    while (value != 0);

    return out;
}

std::string
format_dec(
    u64 _value
)
{
    std::string out;
    u64         value = _value;

    do
    {
        out.insert(out.begin(), static_cast<char>('0' + (value % 10u)));
        value /= 10u;
    }
    while (value != 0);

    return out;
}

std::string
format_signed(
    i64 _value
)
{
    // the magnitude is taken unsigned, so the most negative value works
    if (_value < 0)
    {
        return "-" + format_dec(~static_cast<u64>(_value) + 1u);
    }

    return format_dec(static_cast<u64>(_value));
}

/*
group_digits
  Human-readable groups from the least-significant end. The caller passes
  digits only, without a base prefix, so these spaces never enter C literals
  or persisted numeric strings.
*/
std::string
group_digits(
    std::string_view _digits,
    int              _group
)
{
    if ((_group <= 0) || (_digits.size() <= static_cast<std::size_t>(_group)))
    {
        return std::string(_digits);
    }

    const std::size_t group = static_cast<std::size_t>(_group);
    const std::size_t gaps  = (_digits.size() - 1) / group;
    std::string       result;

    result.reserve(_digits.size() + gaps);

    for (std::size_t i = 0; i < _digits.size(); ++i)
    {
        if ( (i > 0) &&
             ((_digits.size() - i) % group == 0) )
        {
            result += ' ';
        }

        result += _digits[i];
    }

    return result;
}

std::string
format_bin(
    u64  _value,
    int  _width,
    int  _group,
    char _separator
)
{
    const int   width = (clamp_width(_width) == 0) ? 1
                                                   : clamp_width(_width);
    std::string out;

    // most significant digit first; a separator every _group digits
    for (int i = width - 1; i >= 0; --i)
    {
        out += (((_value >> i) & 1u) != 0) ? '1' : '0';

        if ( (_group > 0)          &&
             (i > 0)               &&
             ((i % _group) == 0)   &&
             (_separator != '\0') )
        {
            out += _separator;
        }
    }

    return out;
}

std::string
format_hex(
    const wide& _value,
    int         _digits,
    bool        _upper
)
{
    const char* const digits = _upper ? "0123456789ABCDEF"
                                      : "0123456789abcdef";
    const int         needed = (_value.highest() / 4) + 1;
    const int         count  = std::max({ 1, needed, _digits });
    std::string       out;

    out.reserve(static_cast<std::size_t>(count));

    // the most significant digit first
    for (int i = count - 1; i >= 0; --i)
    {
        out += digits[_value.field(i * 4, 4)];
    }

    return out;
}

/*
runs_text
  The set bits, lowest first, as runs: a run of one is "5", a longer one
"4..7"; the runs are listed with commas and a last "and", and past _most of
them the rest are counted.
*/
std::string
runs_text(
    const wide& _value,
    int         _most
)
{
    std::vector<std::string> runs;
    int                      total = 0;

    for (int i = _value.next(0); i >= 0; )
    {
        int end = i;

        while (_value.test(end + 1))
        {
            ++end;
        }

        ++total;

        if (static_cast<int>(runs.size()) < std::max(1, _most))
        {
            runs.push_back( (end == i)
                                ? format_dec(static_cast<u64>(i))
                                : format_dec(static_cast<u64>(i)) + ".." +
                                      format_dec(static_cast<u64>(end)) );
        }

        i = _value.next(end + 1);
    }

    if (total == 0)
    {
        return "no bits";
    }

    const bool  single = ( (total == 1) &&
                           (runs.front().find("..") == std::string::npos) );
    std::string out    = single ? "bit " : "bits ";

    for (std::size_t i = 0; i < runs.size(); ++i)
    {
        if (i > 0)
        {
            out += ( (i + 1 == runs.size()) &&
                     (static_cast<int>(runs.size()) == total) ) ? " and "
                                                                : ", ";
        }

        out += runs[i];
    }

    if (total > static_cast<int>(runs.size()))
    {
        const int more = total - static_cast<int>(runs.size());

        out += " and " + format_dec(static_cast<u64>(more)) +
               ((more == 1) ? " more run" : " more runs");
    }

    return out;
}


// ------------------------------------------------------------------ parsing

/*
parse_integer
  Overflow is caught before it happens: a digit is added only if the value
times the base, plus the digit, stays within 64 bits.
*/
parsed_integer
parse_integer(
    std::string_view _text
)
{
    parsed_integer result;
    std::size_t    at  = 0;
    std::size_t    end = _text.size();

    // surrounding blanks are not part of the number
    while ( (at < end) &&
            (std::isspace(static_cast<unsigned char>(_text[at]))) )
    {
        ++at;
    }

    while ( (end > at) &&
            (std::isspace(static_cast<unsigned char>(_text[end - 1]))) )
    {
        --end;
    }

    bool negative = false;

    if ( (at < end) &&
         ( (_text[at] == '-') ||
           (_text[at] == '+') ) )
    {
        negative = (_text[at] == '-');
        ++at;
    }

    u64 base = 10;

    // a prefix chooses the base; a lone leading zero means octal, as in C
    if ( (end - at >= 2) &&
         (_text[at] == '0') )
    {
        const char second = static_cast<char>(
            std::tolower(static_cast<unsigned char>(_text[at + 1])));

        if (second == 'x')
        {
            base = 16;
            at  += 2;
        }
        else if (second == 'b')
        {
            base = 2;
            at  += 2;
        }
        else if (second == 'o')
        {
            base = 8;
            at  += 2;
        }
        else if (digit_value(second) >= 0)
        {
            base = 8;
            at  += 1;
        }
    }

    u64  value  = 0;
    bool digits = false;

    for (; at < end; ++at)
    {
        const char each = _text[at];

        // separators between digits are skipped
        if ( (each == '\'') ||
             (each == '_') )
        {
            continue;
        }

        const int digit = digit_value(each);

        // the first non-digit must begin a suffix that runs to the end
        if ( (digit < 0) ||
             (static_cast<u64>(digit) >= base) )
        {
            if ( (digits) &&
                 (is_suffix(_text.substr(at, end - at))) )
            {
                break;
            }

            result.error = (digit < 0)
                ? std::string("unexpected character '") + each + "'"
                : std::string("digit '") + each + "' is not valid in base " +
                      format_dec(base);

            return result;
        }

        if (value > (~u64(0) - static_cast<u64>(digit)) / base)
        {
            result.error = "the value does not fit in 64 bits";

            return result;
        }

        value  = value * base + static_cast<u64>(digit);
        digits = true;
    }

    if (!digits)
    {
        result.error = "no digits";

        return result;
    }

    result.ok       = true;
    result.negative = ( (negative) &&
                        (value != 0) );
    result.value    = negative ? (~value + 1u)
                               : value;

    return result;
}

/*
parse_wide
  The text is read as parse_integer reads it. A power-of-two base places
each digit's bits directly, so a value of any width takes time in
proportion to its length; a decimal one is multiplied and added a digit at
a time. A negative number is the two's complement of a magnitude no wider
than 64 bits, since nothing wider has a sign here.
*/
parsed_wide
parse_wide(
    std::string_view _text
)
{
    parsed_wide result;
    std::size_t at  = 0;
    std::size_t end = _text.size();

    while ( (at < end) &&
            (std::isspace(static_cast<unsigned char>(_text[at]))) )
    {
        ++at;
    }

    while ( (end > at) &&
            (std::isspace(static_cast<unsigned char>(_text[end - 1]))) )
    {
        --end;
    }

    bool negative = false;

    if ( (at < end) &&
         ( (_text[at] == '-') ||
           (_text[at] == '+') ) )
    {
        negative = (_text[at] == '-');
        ++at;
    }

    int base = 10;
    int bits = 0;   // a digit's bits, in a power-of-two base

    if ( (end - at >= 2) &&
         (_text[at] == '0') )
    {
        const char second = static_cast<char>(
            std::tolower(static_cast<unsigned char>(_text[at + 1])));

        if (second == 'x')
        {
            base = 16;
            bits = 4;
            at  += 2;
        }
        else if (second == 'b')
        {
            base = 2;
            bits = 1;
            at  += 2;
        }
        else if (second == 'o')
        {
            base = 8;
            bits = 3;
            at  += 2;
        }
        else if (digit_value(second) >= 0)
        {
            base = 8;
            bits = 3;
            at  += 1;
        }
    }

    // the digits, most significant first
    std::vector<int> digits;

    for (; at < end; ++at)
    {
        const char each = _text[at];

        if ( (each == '\'') ||
             (each == '_') )
        {
            continue;
        }

        const int digit = digit_value(each);

        if ( (digit < 0) ||
             (digit >= base) )
        {
            if ( (!digits.empty()) &&
                 (is_suffix(_text.substr(at, end - at))) )
            {
                break;
            }

            result.error = (digit < 0)
                ? std::string("unexpected character '") + each + "'"
                : std::string("digit '") + each + "' is not valid in base " +
                      format_dec(static_cast<u64>(base));

            return result;
        }

        digits.push_back(digit);
    }

    if (digits.empty())
    {
        result.error = "no digits";

        return result;
    }

    wide value;

    if (bits > 0)
    {
        // least significant digit first, each at its place
        int place = 0;

        for (std::size_t i = digits.size(); i-- > 0; )
        {
            if (digits[i] != 0)
            {
                if (place + highest_set(static_cast<u64>(digits[i])) >=
                    MAX_WIDTH)
                {
                    result.error = "the value does not fit in " +
                                   format_dec(static_cast<u64>(MAX_WIDTH)) +
                                   " bits";

                    return result;
                }

                value.put(place, bits, static_cast<u64>(digits[i]));
            }

            place += bits;
        }
    }
    else
    {
        for (int digit : digits)
        {
            // ten times a value below 2^(MAX_WIDTH - 4), and a digit, is
            // below 2^MAX_WIDTH
            if (value.highest() >= MAX_WIDTH - 4)
            {
                result.error = "the value does not fit in " +
                               format_dec(static_cast<u64>(MAX_WIDTH)) +
                               " bits";

                return result;
            }

            value = add(multiply(value, 10u, MAX_WIDTH),
                        wide(static_cast<u64>(digit)),
                        MAX_WIDTH);
        }
    }

    if ( (negative) &&
         (value.any()) )
    {
        if (value.highest() >= INTEGER_WIDTH)
        {
            result.error = "a negative number must fit in 64 bits";

            return result;
        }

        result.value    = wide(~value.low() + 1u);
        result.negative = true;
    }
    else
    {
        result.value = value;
    }

    result.ok = true;

    return result;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
