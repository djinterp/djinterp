/******************************************************************************
* djinterp [math]                                                 constant.hpp
*
* 
*
* 
* file:      /inc/djinterp/math/constant.hpp         
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.03
******************************************************************************/

#ifndef DJINTERP_MATH_CONSTANT_
#define	DJINTERP_MATH_CONSTANT_ 1

// std
#include <string>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "./math.hpp"



NS_DJINTERP // djinterp
NS_MATH     // math


// numeric_constant.hpp
//   Arbitrary-precision numeric constant types with cross-type comparison.

#ifndef NUMERIC_CONSTANT_HPP
#define NUMERIC_CONSTANT_HPP


namespace numeric
{

// ============================================================================
// Forward declarations
// ============================================================================

class integer_constant;
class decimal_constant;
class rational_constant;

// ============================================================================
// Internal big integer operations
// ============================================================================

namespace internal
{

// big_cmp
//   function: compares two unsigned integer strings digit-by-digit.
inline constexpr int
big_cmp
(
    std::string_view _a,
    std::string_view _b
);

// big_add
//   function: adds two unsigned integer strings.
inline constexpr std::string
big_add
(
    std::string_view _a,
    std::string_view _b
);

// big_sub
//   function: subtracts _b from _a (unsigned).
inline constexpr std::string
big_sub
(
    std::string_view _a,
    std::string_view _b
);

// big_mul
//   function: multiplies two unsigned integer strings.
inline constexpr std::string
big_mul
(
    std::string_view _a,
    std::string_view _b
);

// big_mul_10
//   function: multiplies an unsigned integer string by 10.
inline constexpr std::string
big_mul_10
(
    std::string_view _a
);

// normalize
//   function: removes leading zeros from an unsigned integer string.
inline constexpr std::string
normalize
(
    std::string_view _s
);

// power_of_10
//   function: returns "1" followed by _n zeros.
inline constexpr std::string
power_of_10
(
    std::size_t _n
);

// pad_right
//   function: pads string with trailing zeros to reach target length.
inline constexpr std::string
pad_right
(
    std::string_view _s,
    std::size_t      _len
);

}  // namespace internal

// ============================================================================
// integer_constant
// ============================================================================

// integer_constant
//   class: arbitrary-precision signed integer.
class integer_constant
{
public:
    constexpr integer_constant();
    constexpr explicit integer_constant(std::string_view _value);
    constexpr integer_constant(std::int64_t _value);

    constexpr bool             is_negative() const;
    constexpr bool             is_zero()     const;
    constexpr std::string_view magnitude()   const;
    constexpr std::string      to_string()   const;

    // comparison operators (same type)
    constexpr std::strong_ordering operator<=>(const integer_constant& _other) const;
    constexpr bool                 operator==(const integer_constant& _other) const;

    // cross-type comparison operators
    constexpr std::strong_ordering operator<=>(const decimal_constant&  _other) const;
    constexpr std::strong_ordering operator<=>(const rational_constant& _other) const;
    constexpr bool                 operator==(const decimal_constant&  _other) const;
    constexpr bool                 operator==(const rational_constant& _other) const;

private:
    std::string m_magnitude;  // absolute value as digit string
    bool        m_negative;
};

// ============================================================================
// decimal_constant
// ============================================================================

// decimal_constant
//   class: arbitrary-precision signed decimal number.
class decimal_constant
{
public:
    constexpr decimal_constant();
    constexpr explicit decimal_constant(std::string_view _value);

    constexpr bool             is_negative()       const;
    constexpr bool             is_zero()           const;
    constexpr std::string_view integer_part()      const;
    constexpr std::string_view fractional_part()   const;
    constexpr std::size_t      fractional_digits() const;
    constexpr std::string      to_string()         const;

    // comparison operators (same type)
    constexpr std::strong_ordering operator<=>(const decimal_constant& _other) const;
    constexpr bool                 operator==(const decimal_constant& _other) const;

    // cross-type comparison operators
    constexpr std::strong_ordering operator<=>(const integer_constant&  _other) const;
    constexpr std::strong_ordering operator<=>(const rational_constant& _other) const;
    constexpr bool                 operator==(const integer_constant&  _other) const;
    constexpr bool                 operator==(const rational_constant& _other) const;

private:
    std::string m_integer_part;
    std::string m_fractional_part;
    bool        m_negative;
};

// ============================================================================
// rational_constant
// ============================================================================

// rational_constant
//   class: arbitrary-precision signed rational number (numerator/denominator).
class rational_constant
{
public:
    constexpr rational_constant();
    constexpr rational_constant(std::string_view _numerator,
                                std::string_view _denominator,
                                bool             _negative = false);
    constexpr explicit rational_constant(std::string_view _value);

    constexpr bool             is_negative() const;
    constexpr bool             is_zero()     const;
    constexpr std::string_view numerator()   const;
    constexpr std::string_view denominator() const;
    constexpr std::string      to_string()   const;

    // comparison operators (same type) - uses cross-multiplication
    constexpr std::strong_ordering operator<=>(const rational_constant& _other) const;
    constexpr bool                 operator==(const rational_constant& _other) const;

    // cross-type comparison operators
    constexpr std::strong_ordering operator<=>(const integer_constant& _other) const;
    constexpr std::strong_ordering operator<=>(const decimal_constant& _other) const;
    constexpr bool                 operator==(const integer_constant& _other) const;
    constexpr bool                 operator==(const decimal_constant& _other) const;

private:
    std::string m_numerator;    // absolute value
    std::string m_denominator;  // absolute value, never zero
    bool        m_negative;
};

// ============================================================================
// Free comparison functions
// ============================================================================

// compare (integer, integer)
//   function: three-way comparison returning -1, 0, or 1.
inline constexpr int
compare
(
    const integer_constant& _a,
    const integer_constant& _b
);

// compare (decimal, decimal)
//   function: three-way comparison returning -1, 0, or 1.
inline constexpr int
compare
(
    const decimal_constant& _a,
    const decimal_constant& _b
);

// compare (rational, rational)
//   function: three-way comparison returning -1, 0, or 1.
inline constexpr int
compare
(
    const rational_constant& _a,
    const rational_constant& _b
);

// compare (integer, decimal)
//   function: cross-type three-way comparison.
inline constexpr int
compare
(
    const integer_constant& _a,
    const decimal_constant& _b
);

// compare (decimal, integer)
//   function: cross-type three-way comparison.
inline constexpr int
compare
(
    const decimal_constant& _a,
    const integer_constant& _b
);

// compare (integer, rational)
//   function: cross-type three-way comparison.
inline constexpr int
compare
(
    const integer_constant&  _a,
    const rational_constant& _b
);

// compare (rational, integer)
//   function: cross-type three-way comparison.
inline constexpr int
compare
(
    const rational_constant& _a,
    const integer_constant&  _b
);

// compare (decimal, rational)
//   function: cross-type three-way comparison.
inline constexpr int
compare
(
    const decimal_constant&  _a,
    const rational_constant& _b
);

// compare (rational, decimal)
//   function: cross-type three-way comparison.
int compare(const rational_constant& _a, const decimal_constant&  _b);


NS_END	// maths
NS_END	// djinterp


#endif	// DJINTERP_MATH_CONSTANT_