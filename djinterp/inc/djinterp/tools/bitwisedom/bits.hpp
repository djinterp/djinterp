/*******************************************************************************
* bitwisedom [bitwisedom]                                               bits.hpp
*
* Bits within a width: masks, runs, rotations, encodings and integer text.
*   Most of what is here works on a value held in the low _width bits of a
* 64-bit unsigned integer, the widest a C or C++ integer literal can spell.
* Bits above the width are ignored on the way in and zero on the way out.
*   A sheet may be wider than any integer -- a bitmap of features, a set of
* hundreds of flags -- up to MAX_WIDTH bits, and its values are `wide`: as
* many 64-bit words as the highest set bit needs. The same movements,
* arithmetic and text work on them, and C holds them as arrays of words.
*   ENCODINGS are how a field holds a signed number: plain unsigned, two's
* complement, ones' complement, sign and magnitude, or offset binary with a
* bias. Each has a range, and encode() says whether a value fits it while
* still producing the bits the value wraps to.
*   LEVELS. The floor is C++17, as it is for uxoxo, which the application
* stands on: below it the file is empty rather than an error.
*
*
* path:      /inc/djinterp/tools/bitwisedom/bits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WIDTHS AND MASKS
    ----------------
    1.  Types and limits
    2.  Masks and runs
    3.  Movement within a width
    4.  Values wider than an integer
2.  ENCODINGS
    ---------
    1.  encoding
    2.  Range, encode, decode
3.  INTEGER TEXT
    ------------
    1.  Formatting
    2.  Parsing
*/

#ifndef BITWISEDOM_BITS_HPP
#define BITWISEDOM_BITS_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// re_std
#include "re_std/cstdint/cstdint.hpp"  // re_std::uint64_t, re_std::int64_t


namespace bitwisedom
{


//==============================================================================
// 1.  WIDTHS AND MASKS
//==============================================================================


// 1.1    Types and limits
//------------------------------------------------------------------------------
// 1.1.1
// u64 / i64
//   type: the unsigned and signed carriers of every value bitwisedom handles.
using u64 = re_std::uint64_t;
using i64 = re_std::int64_t;

// 1.1.2
// INTEGER_WIDTH / MAX_WIDTH
//   constants: the widest C integer, in bits -- the widest value the
// functions on u64 take -- and the widest sheet.
const int INTEGER_WIDTH = 64;
const int MAX_WIDTH     = 65536;

// is_wide
//   whether a sheet of a width is wider than an integer: its values are
// sets of bits, and C holds them as arrays.
inline bool
is_wide(
    int _width
)
{
    return (_width > INTEGER_WIDTH);
}

// 1.2    Masks and runs
//------------------------------------------------------------------------------
// basic masks -- all pure, none allocate; widths outside [0, 64] are clamped
// to it
D_NODISCARD u64  width_mask(int _width);
D_NODISCARD u64  range_mask(int _lsb,
                            int _width);
D_NODISCARD int  bit_count(u64 _value);
D_NODISCARD int  lowest_set(u64 _value);
D_NODISCARD int  highest_set(u64 _value);

/**
 * @brief Whether the set bits of a value form one unbroken run.
 *
 * @param[in]  _value  the value; zero is not a run.
 * @param[out] _lsb    the run's lowest bit, when it is one.
 * @param[out] _width  the run's length, when it is one.
 * @return `true` if _value is a single run of ones, `false` otherwise.
 */
D_NODISCARD bool is_run(u64  _value,
                        int& _lsb,
                        int& _width);

// 1.3    Movement within a width
//------------------------------------------------------------------------------
// movement -- each takes and returns a value in the low _width bits
D_NODISCARD u64  rotate_left(u64 _value,
                             int _count,
                             int _width);
D_NODISCARD u64  rotate_right(u64 _value,
                              int _count,
                              int _width);
D_NODISCARD u64  reverse_bits(u64 _value,
                              int _width);
D_NODISCARD u64  reverse_bytes(u64 _value,
                               int _width);
D_NODISCARD i64  sign_extend(u64 _value,
                             int _width);

// 1.4    Values wider than an integer
//------------------------------------------------------------------------------
// 1.4.1
// wide
//   class: a set of bits, as many 64-bit words as its highest bit needs --
// bit n is bit n % 64 of word n / 64. It has no width of its own: the
// functions that move or count within a width take one, as the u64 ones
// do. A u64 converts to a wide implicitly, so a narrow value is a wide one.
class wide
{
public:
    wide() = default;
    wide(u64 _low);   // implicit: a narrow value is a wide one

    // construction -- one bit, or a run of them
    D_NODISCARD static wide  bit(int _index);
    D_NODISCARD static wide  run(int _lsb,
                                 int _width);

    // bits -- out-of-range indices read as clear and are not set
    D_NODISCARD bool  test(int _index) const;
    void              set(int  _index,
                          bool _on = true);

    // words -- word 0 holds bits 0..63; past the last, zero
    D_NODISCARD u64  word(int _index) const;
    D_NODISCARD u64  low() const;
    D_NODISCARD int  words() const;

    // fields -- up to 64 bits from _lsb, read as an integer and written
    // from one; slice is any run, as a wide value moved down to bit 0
    D_NODISCARD u64   field(int _lsb,
                            int _width) const;
    void              put(int _lsb,
                          int _width,
                          u64 _bits);
    D_NODISCARD wide  slice(int _lsb,
                            int _width) const;

    // counting -- lowest and highest are -1, and next is -1 past the last,
    // when no bit is set
    D_NODISCARD bool  any() const;
    D_NODISCARD int   count() const;
    D_NODISCARD int   lowest() const;
    D_NODISCARD int   highest() const;
    D_NODISCARD int   next(int _from) const;

    // width -- whether every set bit lies below it, and the bits above it
    // cleared
    D_NODISCARD bool  fits(int _width) const;
    void              trim(int _width);

    wide&  operator|=(const wide& _other);
    wide&  operator&=(const wide& _other);
    wide&  operator^=(const wide& _other);

    D_NODISCARD bool  operator==(const wide& _other) const;
    D_NODISCARD bool  operator!=(const wide& _other) const;

private:
    std::vector<u64> m_words;   // never a zero word at the top

    void normalize();
};

// the bitwise operators, by value
D_NODISCARD wide  operator|(wide        _a,
                            const wide& _b);
D_NODISCARD wide  operator&(wide        _a,
                            const wide& _b);
D_NODISCARD wide  operator^(wide        _a,
                            const wide& _b);

// movement and arithmetic within a width -- each takes and returns a value
// whose set bits lie below _width
D_NODISCARD wide  complement(const wide& _value,
                             int         _width);
D_NODISCARD wide  shift_left(const wide& _value,
                             int         _count,
                             int         _width);
D_NODISCARD wide  shift_right(const wide& _value,
                              int         _count);
D_NODISCARD wide  rotate_left(const wide& _value,
                              int         _count,
                              int         _width);
D_NODISCARD wide  rotate_right(const wide& _value,
                               int         _count,
                               int         _width);
D_NODISCARD wide  reverse_bits(const wide& _value,
                               int         _width);
D_NODISCARD wide  reverse_bytes(const wide& _value,
                                int         _width);
D_NODISCARD wide  add(const wide& _a,
                      const wide& _b,
                      int         _width);
D_NODISCARD wide  subtract(const wide& _a,
                           const wide& _b,
                           int         _width);
D_NODISCARD wide  negate(const wide& _value,
                         int         _width);
D_NODISCARD wide  multiply(const wide& _value,
                           u64         _by,
                           int         _width);

// a signed number as the _width bits of its two's complement
D_NODISCARD wide  widen_signed(i64 _value,
                               int _width);

/**
 * @brief Whether the set bits of a wide value form one unbroken run.
 *
 * @param[in]  _value  the value; zero is not a run.
 * @param[out] _lsb    the run's lowest bit, when it is one.
 * @param[out] _width  the run's length, when it is one.
 * @return `true` if _value is a single run of ones, `false` otherwise.
 */
D_NODISCARD bool  is_run(const wide& _value,
                         int&        _lsb,
                         int&        _width);


//==============================================================================
// 2.  ENCODINGS
//==============================================================================


// 2.1    encoding
//------------------------------------------------------------------------------
// 2.1.1
// encoding
//   enum: how a field's bits hold a number.
enum class encoding : int
{
    unsigned_binary = 0,
    twos_complement,
    ones_complement,
    sign_magnitude,
    offset_binary
};

// 2.1.2
// ENCODING_COUNT
//   constant: how many encodings there are.
const int ENCODING_COUNT = 5;

// 2.2    Range, encode, decode
//------------------------------------------------------------------------------
// 2.2.1
// value_range
//   struct: the smallest and largest number an encoding holds in a width.
struct value_range
{
    i64 low  = 0;
    i64 high = 0;
};

D_NODISCARD const char*  encoding_name(encoding _encoding);
D_NODISCARD i64          default_bias(int _width);
D_NODISCARD value_range  range_of(encoding _encoding,
                                  int      _width,
                                  i64      _bias);

/**
 * @brief Encodes a number into a field's bits.
 *
 * @param[in]  _encoding  how the field holds numbers.
 * @param[in]  _width     the field's width in bits.
 * @param[in]  _bias      the bias, for offset binary; ignored otherwise.
 * @param[in]  _value     the number to encode.
 * @param[out] _bits      the field's bits: _value's encoding when it fits,
 *                        and the bits it wraps to when it does not.
 * @return `true` if _value lies in the encoding's range, `false` otherwise.
 */
D_NODISCARD bool  encode(encoding _encoding,
                         int      _width,
                         i64      _bias,
                         i64      _value,
                         u64&     _bits);
D_NODISCARD i64   decode(encoding _encoding,
                         int      _width,
                         i64      _bias,
                         u64      _bits);


//==============================================================================
// 3.  INTEGER TEXT
//==============================================================================


// 3.1    Formatting
//------------------------------------------------------------------------------
// formatting -- digits only: no prefix and no suffix; _digits pads with zeros
D_NODISCARD std::string  format_hex(u64  _value,
                                    int  _digits,
                                    bool _upper);
D_NODISCARD std::string  format_oct(u64 _value);
D_NODISCARD std::string  format_dec(u64 _value);
D_NODISCARD std::string  format_signed(i64 _value);
// For display only: separate unprefixed digits from the right with spaces.
D_NODISCARD std::string  group_digits(std::string_view _digits,
                                      int              _group);
D_NODISCARD std::string  format_bin(u64  _value,
                                    int  _width,
                                    int  _group,
                                    char _separator);
D_NODISCARD std::string  format_hex(const wide& _value,
                                    int         _digits,
                                    bool        _upper);

/**
 * @brief A set of bits as its runs, for a person to read.
 *
 * @param[in] _value  the bits.
 * @param[in] _most   how many runs to name before saying how many more.
 * @return "no bits", "bit 5", or "bits 4..7, 12 and 30..31", lowest first;
 *         past _most runs, "and 3 more runs".
 */
D_NODISCARD std::string  runs_text(const wide& _value,
                                   int         _most);

// 3.2    Parsing
//------------------------------------------------------------------------------
// 3.2.1
// parsed_integer
//   struct: what came of reading an integer from text. `value` is the
// number's 64-bit pattern: a negative number is held as two's complement.
struct parsed_integer
{
    bool        ok       = false;
    u64         value    = 0;
    bool        negative = false;
    std::string error;
};

/**
 * @brief Reads an integer as C spells one, more forgivingly.
 *
 * @note Accepts an optional sign; `0x`, `0b`, `0o` and a leading `0`
 *       (octal, as in C); `'` and `_` between digits; and the suffixes `u`,
 *       `l`, `ul`, `ll` and `ull` in either case, which it ignores.
 *
 * @param[in] _text  the text, with surrounding blanks allowed.
 * @return the value, or `ok == false` and why: no digits, a digit the base
 *         lacks, or a value beyond 64 bits.
 */
D_NODISCARD parsed_integer parse_integer(std::string_view _text);

// 3.2.2
// parsed_wide
//   struct: what came of reading a number of any width from text. A
// negative number is held as its 64-bit two's complement, and so is no
// wider than 64 bits.
struct parsed_wide
{
    bool        ok       = false;
    wide        value;
    bool        negative = false;
    std::string error;
};

/**
 * @brief Reads a number as parse_integer does, of any width up to
 *        MAX_WIDTH bits.
 *
 * @param[in] _text  the text, with surrounding blanks allowed.
 * @return the value, or `ok == false` and why.
 */
D_NODISCARD parsed_wide parse_wide(std::string_view _text);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_BITS_HPP
