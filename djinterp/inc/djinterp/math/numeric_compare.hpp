// numeric_compare.hpp
//   Arbitrary-precision numeric string comparison supporting integers, decimals,
//   fractions (proper and mixed), and scientific notation.

#ifndef NUMERIC_COMPARE_HPP
#define NUMERIC_COMPARE_HPP

#include <string>
#include <string_view>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
// djinterp
#include "..\djinterp.h"


NS_DJINTERP  // djinterp
NS_MATH      // math

NS_INTERNAL
    int         big_cmp(std::string_view _a, std::string_view _b);
    std::string big_add(std::string_view _a, std::string_view _b);
    std::string big_sub(std::string_view _a, std::string_view _b);
    std::string big_mul(std::string_view _a, std::string_view _b);
    std::string big_mul_10(std::string_view _a);
    std::string normalize(std::string_view _s);
    std::string power_of_10(std::size_t _n);
    std::string pad_right(std::string_view _s, std::size_t _len);
NS_END  // internal

// integer_value
//   class: arbitrary-precision unsigned integer for internal calculations.
class integer_value
{
public:
    using size_type = std::size_t;

    integer_value();
    integer_value(std::string_view _digits);
    integer_value(std::uint64_t    _value);

    bool        is_zero()                            const;
    std::string to_string()                          const;
    int         compare(const integer_value& _other) const;

    integer_value  operator+ (const integer_value& _other) const;
    integer_value  operator- (const integer_value& _other) const;
    integer_value  operator* (const integer_value& _other) const;
    integer_value& operator+=(const integer_value& _other);
    bool           operator< (const integer_value& _other) const;
    bool           operator> (const integer_value& _other) const;
    bool           operator==(const integer_value& _other) const;
    bool           operator<=(const integer_value& _other) const;
    bool           operator>=(const integer_value& _other) const;

private:
    void m_normalize();
    
    std::string m_digits;  // stored in reverse order (least significant first)
};

// rational
//   class: arbitrary-precision signed rational number (numerator/denominator).
class rational_value
{
public:
    rational();
    rational(integer_value _numerator,
             integer_value _denominator,
             bool        _negative);

    int compare(const rational& _other) const;

    bool is_negative() const;
    bool is_zero()     const;

private:
    integer_value m_numerator;
    integer_value m_denominator;
    bool        m_negative;
};

// parse_number
//   function: parses a numeric string into a rational representation.
rational parse_number(std::string_view _input);

// comparison functions
int compare(std::string_view _a,
            std::string_view _b);

bool equal(std::string_view _a,
           std::string_view _b);

bool not_equal(std::string_view _a,
               std::string_view _b);

bool less_than(std::string_view _a,
               std::string_view _b);

bool greater_than(std::string_view _a,
                  std::string_view _b);

bool less_than_or_equal(std::string_view _a,
                        std::string_view _b);

bool greater_than_or_equal(std::string_view _a,
                           std::string_view _b);


NS_END  // math
NS_END  // djinterp


#endif  // NUMERIC_COMPARE_HPP
