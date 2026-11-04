/*******************************************************************************
* djinterp [cpp/math]                                        numeric_compare.hpp
*
* Numbers written as text, compared exactly at any length: integers,
* decimals, scientific notation, fractions and mixed numbers.
*   THE TEXT is one literal as c/math/literal.h reads it, with an optional
* sign: 42, -0.5, 6.02e23, 2/3, 1 1/2. A numeric_text is a pointer and a
* length, made from a C string, a std::string, or from C++17 a
* std::string_view; it needs no NUL.
*   THE COMPARISON is exact, whatever the number of digits or the size of the
* exponent: 0.1 is below 1/9, 1/3 is above 0.3 followed by a thousand 3s,
* and 1e100000000000000000000 is below 2e100000000000000000000. compare gives
* -1, 0 or 1, the C core's D_MATH_VALUE_LESS, _EQUAL and _GREATER; a text
* that is not one literal, or a fraction over 0, is not a number and is
* unordered with everything, itself included, as a NaN is:
* D_MATH_VALUE_UNORDERED, 2. So equal and the four orderings are false for
* it, and not_equal is true.
*   THE NUMBERS: integer_value is a natural number of any size;
* rational_value a signed rational number of any size, times a power of ten
* of any size, so that 1e1000000 is held in a few digits; parse_number
* reads a literal as one. Their digits are decimal, in a std::string, so they
* allocate: until bignum (roadmap P1) the C core has no arbitrary precision
* of its own. Nothing here throws, but std::string may throw
* std::bad_alloc.
*   COST. Orders of magnitude settle most comparisons, without multiplying,
* and usual literals compare in about half a microsecond; literal.h and
* value.h read and compare those that are exact in the C core's family in a
* third of that or less. Only numbers within a factor of 100 of each other are
* cross-multiplied, in time that grows with the product of their digit
* counts: two fractions with terms of 1,000 digits take a few milliseconds.
*   ONE INTERFACE AT EVERY LEVEL from C++98. From C++11 the values move.
*
*
* path:      /inc/djinterp/cpp/modules/math/numeric_compare.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TEXT
    ----
    1.  numeric_text
2.  INTEGERS
    --------
    1.  integer_value
    2.  Construction and queries
    3.  Arithmetic
    4.  Comparison
3.  RATIONALS
    ---------
    1.  Signed integers of any size
    2.  rational_value
    3.  Construction and queries
    4.  Comparison
4.  READING
    -------
    1.  parse_number
5.  COMPARING TEXTS
    ---------------
    1.  compare
    2.  The predicates
*/

#ifndef DJINTERP_MATH_NUMERIC_COMPARE_HPP
#define DJINTERP_MATH_NUMERIC_COMPARE_HPP 1

// std
#include <algorithm>                // std::reverse
#include <cstddef>                  // std::size_t
#include <cstring>                  // std::strlen
#include <string>                   // std::string
// djinterp
#include "../../djinterp.hpp"                 // framework root
#include "../../../c/modules/math/literal.h"  // d_math_literal_length, and
                                              // d_math_value_order through value.h
#include "./math_common.hpp"                  // NS_MATH
// std, from C++17 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>          // std::string_view
#endif


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  TEXT
//==============================================================================


// 1.1    numeric_text
//------------------------------------------------------------------------------
// numeric_text
//   class: a number's text, read in place: a pointer and a length. It owns
// nothing, so what it points into must outlive it. A null C string is the
// empty text.
class numeric_text
{
public:
    numeric_text(const char* _text);
    numeric_text(const char* _text,
                 std::size_t _length);
    numeric_text(const std::string& _text);
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    numeric_text(std::string_view _text);
#endif

    const char* data() const;
    std::size_t size() const;

private:
    const char* m_data;
    std::size_t m_size;
};

inline
numeric_text::numeric_text(
    const char* _text
)
    : m_data((_text != D_NULLPTR) ? _text : ""),
      m_size((_text != D_NULLPTR) ? std::strlen(_text) : 0u)
{}

inline
numeric_text::numeric_text(
    const char* _text,
    std::size_t _length
)
    : m_data((_text != D_NULLPTR) ? _text : ""),
      m_size((_text != D_NULLPTR) ? _length : 0u)
{}

inline
numeric_text::numeric_text(
    const std::string& _text
)
    : m_data(_text.data()),
      m_size(_text.size())
{}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

inline
numeric_text::numeric_text(
    std::string_view _text
)
    : m_data((_text.data() != D_NULLPTR) ? _text.data() : ""),
      m_size(_text.size())
{}

#endif

inline const char*
numeric_text::data() const
{
    return m_data;
}

inline std::size_t
numeric_text::size() const
{
    return m_size;
}


//==============================================================================
// 2.  INTEGERS
//==============================================================================


class rational_value;

// 2.1    integer_value
//------------------------------------------------------------------------------
// integer_value
//   class: a natural number of any size. Its decimal digits are characters
// in a std::string, the least significant first, with no zero above the
// most significant nonzero digit, so 0 has none.
class integer_value
{
public:
    typedef std::size_t size_type;

    integer_value();
    integer_value(d_math_umax _value);
    integer_value(numeric_text _digits);

    bool        is_zero() const;
    size_type   digits() const;
    std::string to_string() const;
    int         compare(const integer_value& _other) const;

    integer_value  operator+(const integer_value& _other) const;
    integer_value  operator-(const integer_value& _other) const;
    integer_value  operator*(const integer_value& _other) const;
    integer_value& operator+=(const integer_value& _other);
    bool           operator<(const integer_value& _other) const;
    bool           operator>(const integer_value& _other) const;
    bool           operator==(const integer_value& _other) const;
    bool           operator!=(const integer_value& _other) const;
    bool           operator<=(const integer_value& _other) const;
    bool           operator>=(const integer_value& _other) const;

private:
    friend class rational_value;

    void      m_trim();
    size_type m_low_zeros() const;
    void      m_shift_up(size_type _places);
    void      m_shift_down(size_type _places);

    std::string m_digits;
};

// 2.2    Construction and queries
//------------------------------------------------------------------------------
// integer_value()
//   0.
inline
integer_value::integer_value()
    : m_digits()
{}

// integer_value(d_math_umax)
//   the value of an unsigned integer, the widest the build can spell.
inline
integer_value::integer_value(
    d_math_umax _value
)
    : m_digits()
{
    d_math_umax rest = _value;

    // each digit, the least significant first
    while (rest != 0u)
    {
        m_digits.push_back(static_cast<char>('0' + static_cast<int>(rest %
                                                                    10u)));
        rest = rest / 10u;
    }
}

// integer_value(numeric_text)
//   the number that the digits at the start of _digits make, as strtoul
// reads one, without a sign or a space: 0 where it starts with none.
inline
integer_value::integer_value(
    numeric_text _digits
)
    : m_digits()
{
    const char* const text  = _digits.data();
    std::size_t       count = 0u;

    // the digits at the start
    while ( (count < _digits.size()) &&
            (text[count] >= '0')     &&
            (text[count] <= '9') )
    {
        ++count;
    }

    m_digits.assign(text, count);
    std::reverse(m_digits.begin(), m_digits.end());
    m_trim();
}

// is_zero
//   whether this is 0.
inline bool
integer_value::is_zero() const
{
    return m_digits.empty();
}

// digits
//   how many decimal digits this has: 1 for 0.
inline integer_value::size_type
integer_value::digits() const
{
    return (m_digits.empty()) ? 1u : m_digits.size();
}

// to_string
//   the decimal digits, the most significant first: "0" for 0.
inline std::string
integer_value::to_string() const
{
    std::string result(m_digits.rbegin(), m_digits.rend());

    // 0
    if (result.empty())
    {
        result = "0";
    }

    return result;
}

// m_trim
//   drops the zeros above the most significant nonzero digit.
inline void
integer_value::m_trim()
{
    std::string::size_type length = m_digits.size();

    // the zeros at the top
    while ( (length > 0u) &&
            (m_digits[length - 1u] == '0') )
    {
        --length;
    }

    m_digits.resize(length);

    return;
}

// m_low_zeros
//   how many zeros this ends with: 0 for 0.
inline integer_value::size_type
integer_value::m_low_zeros() const
{
    size_type count = 0u;

    // the zeros at the bottom
    while ( (count < m_digits.size()) &&
            (m_digits[count] == '0') )
    {
        ++count;
    }

    return count;
}

// m_shift_up
//   this times 10^_places.
inline void
integer_value::m_shift_up(
    size_type _places
)
{
    // 0 stays 0
    if (!m_digits.empty())
    {
        m_digits.insert(static_cast<std::string::size_type>(0u),
                        _places,
                        '0');
    }

    return;
}

// m_shift_down
//   this divided by 10^_places, which it must end with that many zeros of.
inline void
integer_value::m_shift_down(
    size_type _places
)
{
    m_digits.erase(0u, _places);

    return;
}

// 2.3    Arithmetic
//------------------------------------------------------------------------------
// operator+=
//   adds _other, which may be this.
inline integer_value&
integer_value::operator+=(
    const integer_value& _other
)
{
    const size_type length = (m_digits.size() > _other.m_digits.size())
                                 ? m_digits.size()
                                 : _other.m_digits.size();
    unsigned        carry  = 0u;
    size_type       i      = 0u;

    m_digits.resize(length, '0');

    // each digit, from the least significant
    for (i = 0u; i < length; ++i)
    {
        const unsigned other = (i < _other.m_digits.size())
                                   ? static_cast<unsigned>(_other.m_digits[i] -
                                                           '0')
                                   : 0u;
        const unsigned sum   = static_cast<unsigned>(m_digits[i] - '0') +
                               other + carry;

        m_digits[i] = static_cast<char>('0' + static_cast<int>(sum % 10u));
        carry       = sum / 10u;
    }

    // a carry out of the top
    if (carry != 0u)
    {
        m_digits.push_back(static_cast<char>('0' + static_cast<int>(carry)));
    }

    return *this;
}

// operator+
//   the sum.
inline integer_value
integer_value::operator+(
    const integer_value& _other
) const
{
    integer_value result(*this);

    result += _other;

    return result;
}

// operator-
//   the difference, where _other is not more than this; 0 where it is.
inline integer_value
integer_value::operator-(
    const integer_value& _other
) const
{
    // _other at least this: 0
    if (compare(_other) <= 0)
    {
        return integer_value();
    }

    integer_value result(*this);
    int           borrow = 0;
    size_type     i      = 0u;

    // each digit, from the least significant
    for (i = 0u; i < result.m_digits.size(); ++i)
    {
        const int other = (i < _other.m_digits.size())
                              ? (_other.m_digits[i] - '0')
                              : 0;
        int       digit = (result.m_digits[i] - '0') - other - borrow;

        borrow             = (digit < 0) ? 1 : 0;
        digit              = digit + (10 * borrow);
        result.m_digits[i] = static_cast<char>('0' + digit);
    }

    result.m_trim();

    return result;
}

// operator*
//   the product, by long multiplication: time in the product of the two
// numbers' digits.
inline integer_value
integer_value::operator*(
    const integer_value& _other
) const
{
    integer_value result;

    // a factor of 0
    if ( (m_digits.empty()) ||
         (_other.m_digits.empty()) )
    {
        return result;
    }

    const size_type count = m_digits.size();
    const size_type other = _other.m_digits.size();
    size_type       i     = 0u;
    size_type       j     = 0u;

    result.m_digits.assign(count + other, '0');

    // each digit of this, times all of _other, added in at its place
    for (i = 0u; i < count; ++i)
    {
        const unsigned digit = static_cast<unsigned>(m_digits[i] - '0');
        unsigned       carry = 0u;

        // each digit of _other; a carry above them lands on a place no
        // earlier row reached
        for (j = 0u; j < other; ++j)
        {
            const unsigned sum =
                static_cast<unsigned>(result.m_digits[i + j] - '0') +
                (digit * static_cast<unsigned>(_other.m_digits[j] - '0')) +
                carry;

            result.m_digits[i + j] =
                static_cast<char>('0' + static_cast<int>(sum % 10u));
            carry                  = sum / 10u;
        }

        result.m_digits[i + other] =
            static_cast<char>('0' + static_cast<int>(carry));
    }

    result.m_trim();

    return result;
}

// 2.4    Comparison
//------------------------------------------------------------------------------
// compare
//   -1, 0 or 1 as this is less than, equal to or greater than _other.
inline int
integer_value::compare(
    const integer_value& _other
) const
{
    // more digits, more
    if (m_digits.size() != _other.m_digits.size())
    {
        return (m_digits.size() < _other.m_digits.size()) ? -1 : 1;
    }

    size_type i = m_digits.size();

    // the most significant digit that differs
    while (i > 0u)
    {
        --i;

        // this digit decides
        if (m_digits[i] != _other.m_digits[i])
        {
            return (m_digits[i] < _other.m_digits[i]) ? -1 : 1;
        }
    }

    return 0;
}

inline bool
integer_value::operator<(
    const integer_value& _other
) const
{
    return (compare(_other) < 0);
}

inline bool
integer_value::operator>(
    const integer_value& _other
) const
{
    return (compare(_other) > 0);
}

inline bool
integer_value::operator==(
    const integer_value& _other
) const
{
    return (m_digits == _other.m_digits);
}

inline bool
integer_value::operator!=(
    const integer_value& _other
) const
{
    return (m_digits != _other.m_digits);
}

inline bool
integer_value::operator<=(
    const integer_value& _other
) const
{
    return (compare(_other) <= 0);
}

inline bool
integer_value::operator>=(
    const integer_value& _other
) const
{
    return (compare(_other) >= 0);
}


//==============================================================================
// 3.  RATIONALS
//==============================================================================


// 3.1    Signed integers of any size
//------------------------------------------------------------------------------
// A rational_value's power of ten, and the orders of magnitude its
// comparison works out, are integers of any size with a sign.

NS_INTERNAL

    // 3.1.1
    // numeric_signed
    //   struct: an integer of any size, as a magnitude and a sign; 0 is
    // never negative.
    struct numeric_signed
    {
        numeric_signed()
            : magnitude(),
              negative(false)
        {}

        numeric_signed(
            const integer_value& _magnitude,
            bool                 _negative
        )
            : magnitude(_magnitude),
              negative( (_negative) &&
                        (!_magnitude.is_zero()) )
        {}

        integer_value magnitude;
        bool          negative;
    };

    // 3.1.2
    // numeric_integer_of_size
    //   the integer_value of a size_t, which may be wider than d_math_umax.
    inline integer_value
    numeric_integer_of_size(
        std::size_t _count
    )
    {
        char        text[64];
        std::size_t at   = sizeof(text);
        std::size_t rest = _count;

        // each digit, the least significant first, from the buffer's end
        do
        {
            --at;
            text[at] = static_cast<char>('0' + static_cast<int>(rest % 10u));
            rest     = rest / 10u;
        } while (rest != 0u);

        return integer_value(numeric_text(text + at,
                                          sizeof(text) - at));
    }

    // 3.1.3
    // numeric_size_of
    //   _x as a size_t, into *_out: false, storing nothing, where it does
    // not fit.
    inline bool
    numeric_size_of(
        const integer_value& _x,
        std::size_t*         _out
    )
    {
        const std::string text   = _x.to_string();
        const std::size_t most   = static_cast<std::size_t>(-1);
        std::size_t       result = 0u;
        std::size_t       i      = 0u;

        // each digit, the most significant first
        for (i = 0u; i < text.size(); ++i)
        {
            const std::size_t digit =
                static_cast<std::size_t>(text[i] - '0');

            // past a size_t
            if (result > ((most - digit) / 10u))
            {
                return false;
            }

            result = (result * 10u) + digit;
        }

        *_out = result;

        return true;
    }

    // 3.1.4
    // numeric_signed_add
    //   _a + _b.
    inline numeric_signed
    numeric_signed_add(
        const numeric_signed& _a,
        const numeric_signed& _b
    )
    {
        // one sign: the magnitudes add
        if (_a.negative == _b.negative)
        {
            return numeric_signed(_a.magnitude + _b.magnitude,
                                  _a.negative);
        }

        // opposite signs: the larger magnitude's sign, and the difference
        if (_a.magnitude.compare(_b.magnitude) >= 0)
        {
            return numeric_signed(_a.magnitude - _b.magnitude,
                                  _a.negative);
        }

        return numeric_signed(_b.magnitude - _a.magnitude,
                              _b.negative);
    }

    // 3.1.5
    // numeric_signed_sub
    //   _a - _b.
    inline numeric_signed
    numeric_signed_sub(
        const numeric_signed& _a,
        const numeric_signed& _b
    )
    {
        return numeric_signed_add(_a,
                                  numeric_signed(_b.magnitude,
                                                 !_b.negative));
    }

NS_END  // internal

// 3.2    rational_value
//------------------------------------------------------------------------------
class rational_value;

// parse_number, declared here for rational_value to befriend; section 4
// defines it.
inline rational_value parse_number(numeric_text _text);

// rational_value
//   class: a signed rational number of any size, held as n / d times 10^k:
// n and d natural numbers of any size, d not 0, and k an integer of any
// size. A zero has no sign. Or not a number: a text that is not one
// literal, or a fraction whose denominator is 0.
class rational_value
{
public:
    rational_value();
    rational_value(const integer_value& _numerator,
                   const integer_value& _denominator,
                   bool                 _negative);

    int  compare(const rational_value& _other) const;
    bool is_negative() const;
    bool is_zero() const;
    bool is_number() const;

private:
    friend rational_value parse_number(numeric_text _text);

    void                     m_normalize();
    int                      m_sign() const;
    internal::numeric_signed m_order() const;
    int                      m_compare_magnitude(
                                 const rational_value& _other) const;

    integer_value            m_numerator;
    integer_value            m_denominator;
    internal::numeric_signed m_exponent;
    bool                     m_negative;
    bool                     m_number;
};

// 3.3    Construction and queries
//------------------------------------------------------------------------------
// rational_value()
//   0.
inline
rational_value::rational_value()
    : m_numerator(),
      m_denominator(static_cast<d_math_umax>(1u)),
      m_exponent(),
      m_negative(false),
      m_number(true)
{}

// rational_value(integer_value, integer_value, bool)
//   _numerator / _denominator, negative where _negative is true and the
// numerator is not 0; over 0, not a number.
inline
rational_value::rational_value(
    const integer_value& _numerator,
    const integer_value& _denominator,
    bool                 _negative
)
    : m_numerator(_numerator),
      m_denominator(_denominator),
      m_exponent(),
      m_negative(_negative),
      m_number(true)
{
    m_normalize();
}

// m_normalize
//   over 0, not a number; 0 with no sign; otherwise the zeros that the
// numerator and the denominator end with moved into the power of ten.
inline void
rational_value::m_normalize()
{
    // over 0
    if (m_denominator.is_zero())
    {
        m_number   = false;
        m_negative = false;

        return;
    }

    // 0
    if (m_numerator.is_zero())
    {
        m_negative    = false;
        m_denominator = integer_value(static_cast<d_math_umax>(1u));
        m_exponent    = internal::numeric_signed();

        return;
    }

    const integer_value::size_type up   = m_numerator.m_low_zeros();
    const integer_value::size_type down = m_denominator.m_low_zeros();

    m_numerator.m_shift_down(up);
    m_denominator.m_shift_down(down);
    m_exponent = internal::numeric_signed_add(
                     m_exponent,
                     internal::numeric_signed(
                         internal::numeric_integer_of_size(up),
                         false));
    m_exponent = internal::numeric_signed_sub(
                     m_exponent,
                     internal::numeric_signed(
                         internal::numeric_integer_of_size(down),
                         false));

    return;
}

// is_negative
//   whether this is a number below 0.
inline bool
rational_value::is_negative() const
{
    return ( (m_number) &&
             (m_negative) );
}

// is_zero
//   whether this is the number 0.
inline bool
rational_value::is_zero() const
{
    return ( (m_number) &&
             (m_numerator.is_zero()) );
}

// is_number
//   whether this is a number: false for a text that was not one literal,
// and for a fraction over 0.
inline bool
rational_value::is_number() const
{
    return m_number;
}

// 3.4    Comparison
//------------------------------------------------------------------------------
// compare
//   -1, 0 or 1 as this is less than, equal to or greater than _other,
// exactly: D_MATH_VALUE_LESS, _EQUAL and _GREATER. Where either is not a
// number, unordered: D_MATH_VALUE_UNORDERED, 2.
inline int
rational_value::compare(
    const rational_value& _other
) const
{
    // not a number: unordered, as a NaN is
    if ( (!m_number) ||
         (!_other.m_number) )
    {
        return static_cast<int>(D_MATH_VALUE_UNORDERED);
    }

    const int sign       = m_sign();
    const int other_sign = _other.m_sign();

    // the signs differ, or both are 0
    if ( (sign != other_sign) ||
         (sign == 0) )
    {
        return (sign < other_sign) ? static_cast<int>(D_MATH_VALUE_LESS)
             : (sign > other_sign) ? static_cast<int>(D_MATH_VALUE_GREATER)
                                   : static_cast<int>(D_MATH_VALUE_EQUAL);
    }

    const int magnitude = m_compare_magnitude(_other);

    return (sign < 0) ? -magnitude : magnitude;
}

// m_sign
//   -1, 0 or 1: this number's sign.
inline int
rational_value::m_sign() const
{
    // 0
    if (m_numerator.is_zero())
    {
        return 0;
    }

    return (m_negative) ? -1 : 1;
}

// m_order
//   k + digits(n) - digits(d): a nonzero number's magnitude lies strictly
// between 10^(m_order() - 1) and 10^(m_order() + 1).
inline internal::numeric_signed
rational_value::m_order() const
{
    const internal::numeric_signed up(
        internal::numeric_integer_of_size(m_numerator.digits()),
        false);
    const internal::numeric_signed down(
        internal::numeric_integer_of_size(m_denominator.digits()),
        false);

    return internal::numeric_signed_sub(
               internal::numeric_signed_add(m_exponent, up),
               down);
}

// m_compare_magnitude
//   -1, 0 or 1 as this nonzero number's magnitude is less than, equal to or
// greater than _other's. Orders of magnitude two or more apart decide it.
// Otherwise the two are cross-multiplied, the power of ten that one has
// over the other moved across as zeros: then k1 - k2 is the orders'
// difference, at most 1, plus the digit counts' differences, so the zeros
// are no more than the digits already held.
inline int
rational_value::m_compare_magnitude(
    const rational_value& _other
) const
{
    const internal::numeric_signed apart =
        internal::numeric_signed_sub(_other.m_order(), m_order());

    // orders two or more apart
    if (apart.magnitude.compare(
            integer_value(static_cast<d_math_umax>(2u))) >= 0)
    {
        return (apart.negative) ? 1 : -1;
    }

    const internal::numeric_signed shift =
        internal::numeric_signed_sub(m_exponent, _other.m_exponent);
    integer_value left   = m_numerator * _other.m_denominator;
    integer_value right  = _other.m_numerator * m_denominator;
    std::size_t   places = 0u;

    (void)internal::numeric_size_of(shift.magnitude, &places);

    // the larger power of ten's surplus, as zeros on its side
    if (shift.negative)
    {
        right.m_shift_up(places);
    }
    else
    {
        left.m_shift_up(places);
    }

    return left.compare(right);
}


//==============================================================================
// 4.  READING
//==============================================================================


// 4.1    parse_number
//------------------------------------------------------------------------------
// parse_number
//   the number _text is: a decimal number, a fraction or a mixed number, as
// literal.h reads one, the whole text, exactly. Not a number where the text
// is not one literal, or is a fraction over 0.
inline rational_value
parse_number(
    numeric_text _text
)
{
    const char* const text   = _text.data();
    const std::size_t length = _text.size();
    rational_value    result;

    // not one literal
    if ( (length == 0u) ||
         (d_math_literal_length(text, length) != length) )
    {
        result.m_number = false;

        return result;
    }

    const std::size_t start = ( (text[0] == '-') ||
                                (text[0] == '+') ) ? 1u : 0u;
    std::size_t       i     = start;

    // the first run of digits
    while ( (i < length) &&
            (text[i] >= '0') &&
            (text[i] <= '9') )
    {
        ++i;
    }

    // a fraction or a mixed number: digits, then a slash or spaces
    if ( (i < length) &&
         ( (text[i] == '/') ||
           (text[i] == ' ') ) )
    {
        const std::size_t whole_end = i;
        std::size_t       num       = i;

        // a mixed number's spaces, then its fraction's numerator
        if (text[i] == ' ')
        {
            // the spaces
            while (text[num] == ' ')
            {
                ++num;
            }
        }
        else
        {
            num = start;
        }

        std::size_t slash = num;

        // the numerator
        while (text[slash] != '/')
        {
            ++slash;
        }

        const integer_value n(numeric_text(text + num, slash - num));
        const integer_value d(numeric_text(text + slash + 1u,
                                           length - slash - 1u));

        result.m_numerator   = n;
        result.m_denominator = d;

        // a mixed number w n/d is (w d + n)/d
        if (num != start)
        {
            const integer_value w(numeric_text(text + start,
                                               whole_end - start));

            result.m_numerator = (w * d) + n;
        }
    }
    else
    {
        const std::size_t whole_end = i;
        std::size_t       part      = i;
        std::size_t       part_end  = i;

        // the digits after a point
        if ( (i < length) &&
             (text[i] == '.') )
        {
            part = i + 1u;
            i    = part;

            // the digits
            while ( (i < length) &&
                    (text[i] >= '0') &&
                    (text[i] <= '9') )
            {
                ++i;
            }

            part_end = i;
        }

        std::string digits(text + start, whole_end - start);

        digits.append(text + part, part_end - part);
        result.m_numerator = integer_value(numeric_text(digits));

        // the point: k = -(digits after it)
        result.m_exponent = internal::numeric_signed(
                                internal::numeric_integer_of_size(
                                    part_end - part),
                                true);

        // an exponent: e or E, a sign, digits
        if (i < length)
        {
            const bool        exponent_negative = (text[i + 1u] == '-');
            const std::size_t first             = ( (text[i + 1u] == '-') ||
                                                    (text[i + 1u] == '+') )
                                                      ? (i + 2u)
                                                      : (i + 1u);

            result.m_exponent = internal::numeric_signed_add(
                                    result.m_exponent,
                                    internal::numeric_signed(
                                        integer_value(numeric_text(
                                            text + first,
                                            length - first)),
                                        exponent_negative));
        }
    }

    result.m_negative = (text[0] == '-');
    result.m_normalize();

    return result;
}


//==============================================================================
// 5.  COMPARING TEXTS
//==============================================================================


// 5.1    compare
//------------------------------------------------------------------------------
// compare
//   -1, 0 or 1 as the number _a is less than, equal to or greater than the
// number _b, exactly: D_MATH_VALUE_LESS, _EQUAL and _GREATER. Where either
// text is not a number, unordered: D_MATH_VALUE_UNORDERED, 2, so a test of
// compare's sign is not a test of order; the predicates below are.
inline int
compare(
    numeric_text _a,
    numeric_text _b
)
{
    return parse_number(_a).compare(parse_number(_b));
}

// 5.2    The predicates
//------------------------------------------------------------------------------
// Each is false where either text is not a number, except not_equal, which
// is true there, as with a NaN.

inline bool
equal(
    numeric_text _a,
    numeric_text _b
)
{
    return (compare(_a, _b) == static_cast<int>(D_MATH_VALUE_EQUAL));
}

inline bool
not_equal(
    numeric_text _a,
    numeric_text _b
)
{
    return (compare(_a, _b) != static_cast<int>(D_MATH_VALUE_EQUAL));
}

inline bool
less_than(
    numeric_text _a,
    numeric_text _b
)
{
    return (compare(_a, _b) == static_cast<int>(D_MATH_VALUE_LESS));
}

inline bool
greater_than(
    numeric_text _a,
    numeric_text _b
)
{
    return (compare(_a, _b) == static_cast<int>(D_MATH_VALUE_GREATER));
}

inline bool
less_than_or_equal(
    numeric_text _a,
    numeric_text _b
)
{
    const int order = compare(_a, _b);

    return ( (order == static_cast<int>(D_MATH_VALUE_LESS)) ||
             (order == static_cast<int>(D_MATH_VALUE_EQUAL)) );
}

inline bool
greater_than_or_equal(
    numeric_text _a,
    numeric_text _b
)
{
    const int order = compare(_a, _b);

    return ( (order == static_cast<int>(D_MATH_VALUE_GREATER)) ||
             (order == static_cast<int>(D_MATH_VALUE_EQUAL)) );
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_NUMERIC_COMPARE_HPP
