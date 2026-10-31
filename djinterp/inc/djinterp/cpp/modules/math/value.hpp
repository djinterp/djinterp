/*******************************************************************************
* djinterp [cpp/math]                                                  value.hpp
*
* The generic value in C++: math::value, one number of any kind the math
* module computes with, over the C core's struct dj_math_value
* (c/math/value.h); and math::inline_value, the kinds that need no box.
*   THE KINDS AND THE TOWER are value.h's: none, an error, a boolean, an
* integer and a rational, which are exact, a float, a double and a long
* double; and boxed kinds, such as the wide integer (c/math/value_wide.h),
* which register with value.h through a table each one fills in. Exact
* operations stay exact; an exact result too wide for dj_math_imax is the
* long double nearest it, flagged inexact (G2); errors are values that pass
* through; comparison is exact across kinds. Every operation here is
* value.h's.
*   OWNERSHIP. A value holds a box as an int holds its number: a copy is one
* holder more of the box (value.h's retain), and a value destroyed or
* overwritten one fewer (release), so a box lives as long as a value holds
* it. From C++11 a value moves, leaving none behind, and a move never
* fails. adopt takes over a C value whose hold is the caller's, as every
* result of value.h's is; detach hands the hold back to C. An inline_value
* never holds a box.
*   NOTHING THROWS: an error is a value, D_MATH_VALUE_ERROR_MEMORY included,
* where a boxed kind had no storage for a result.
*   CONSTRUCTION is implicit from the arithmetic types: a signed integer is
* an integer; an unsigned one an integer where it fits dj_math_imax, and
* past it G2's long double (d_math_value_from_uint); a float, double or long
* double its own kind; a bool a boolean; and an error code an error. A
* pointer is refused, so that none becomes a boolean. ratio(n, d) is n / d
* exactly, a pole or invalid over 0.
*   ARITHMETIC AND COMPARISON are operators found by argument-dependent
* lookup only: + - * / and their assignments, unary - and +, abs, and the
* six comparisons. A number of another type meeting a value converts to it,
* and no other code sees them. Each comparison asks value.h's exact order,
* so a NaN, an error and unlike kinds are unordered, and every comparison
* with them is false but !=. compare gives the order itself.
*   CONVERSION OUT is to(&out), true where the value has one of that type:
* the nearest float, double or long double of a number, ties to even; the
* rational of an exact one, or the dj_math_imax of an exact integer, where
* it fits; a boolean's truth; an error's code. Where it is false, out is as
* it was.
*   NUMBERS. Both classes are numbers to generic code (number_traits.hpp):
* number_traits gives their zero and one, |x|, the conversions through
* double, a root, exact for the square of a rational of the family, and a
* tolerance 0 where both operands are exact and else four epsilons of the
* larger's kind times their larger magnitude (S158); common_number says an
* inline value with a number stays inline, and with a value becomes one. So
* vectors and matrices of values compute, and compare, exactly.
*   LEVELS. Everything from C++98; from C++11, moves. inline_value is a
* literal type, and its operations are constant expressions from C++14, as
* value.h's are. value, whose destructor releases, is a literal type from
* C++20, where a destructor may be constexpr, and is a constant expression
* there for the inline kinds. From C++20, <=> gives a partial ordering, and
* both classes satisfy the concept number.
*
*
* path:      /inc/djinterp/cpp/modules/math/value.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE INLINE VALUE
    ----------------
    1.  value.h's operations, by type
    2.  inline_value
2.  THE VALUE
    ---------
    1.  value
3.  NUMBERS
    -------
    1.  The square root
    2.  number_traits
    3.  common_number
*/

#ifndef DJINTERP_MATH_VALUE_HPP
#define DJINTERP_MATH_VALUE_HPP 1

// std
#include <cfloat>                // FLT_EPSILON, DBL_EPSILON, LDBL_EPSILON
#include <limits>                // std::numeric_limits
// djinterp
#include "../../djinterp.hpp"       // framework root
#include "../../../c/modules/math/value.h"     // dj_math_value and its operations
#if (defined(__cplusplus))
    #include "../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
#include "./math_common.hpp"     // NS_MATH
#include "./number_traits.hpp"   // number_traits, common_number, number
// std, from C++20 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <compare>           // std::partial_ordering
#endif


NS_DJINTERP
NS_MATH


#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH
D_STATIC_ASSERT( ( (sizeof(long long) <= sizeof(dj_math_imax)) &&
                   (sizeof(unsigned long long) <= sizeof(dj_math_umax)) ),
                "math::value: long long must fit the C core's widest "
                "integers.");
D_LONG_LONG_DIAG_POP
#endif

class value;


//==============================================================================
// 1.  THE INLINE VALUE
//==============================================================================


// 1.1    value.h's operations, by type
//------------------------------------------------------------------------------
// Each class makes an operation's result in place, from value.h's call, with
// no copy of it between (S154). The operation is named by a type, not passed
// as a pointer to the function: value.h's functions are always_inline
// outside testing builds, and g++ cannot always inline a call through a
// pointer, which is then an error.
NS_INTERNAL

    // value_add, value_sub, value_mul, value_div
    //   struct: value.h's operation on two values, as apply.
    struct value_add
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _a,
            const dj_math_value& _b
        ) D_NOEXCEPT
        {
            return d_math_value_add(_a, _b);
        }
    };

    struct value_sub
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _a,
            const dj_math_value& _b
        ) D_NOEXCEPT
        {
            return d_math_value_sub(_a, _b);
        }
    };

    struct value_mul
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _a,
            const dj_math_value& _b
        ) D_NOEXCEPT
        {
            return d_math_value_mul(_a, _b);
        }
    };

    struct value_div
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _a,
            const dj_math_value& _b
        ) D_NOEXCEPT
        {
            return d_math_value_div(_a, _b);
        }
    };

    // value_neg, value_abs
    //   struct: value.h's operation on one value, as apply.
    struct value_neg
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _x
        ) D_NOEXCEPT
        {
            return d_math_value_neg(_x);
        }
    };

    struct value_abs
    {
        static D_CONSTEXPR_CPP14
        dj_math_value apply(
            const dj_math_value& _x
        ) D_NOEXCEPT
        {
            return d_math_value_abs(_x);
        }
    };

NS_END  // internal

// 1.2    inline_value
//------------------------------------------------------------------------------
// inline_value
//   class: a value of an inline kind: none, an error, a boolean, an integer,
// a rational, a float, a double or a long double, never a box. An operation
// on two inline values is an inline value. A literal type, whose operations
// are constant expressions from C++14.
class inline_value
{
private:
    struct from_c_tag
    {};

    // value.h's Operation on _x, its result made in place (1.1): through
    // the constructor that takes a C value, the result would be copied once
    // more, which g++ makes cost more than an integer sum itself. Here,
    // before the operators that use them, as clang reads a template's body
    // only where it stands, and an instantiation made earlier is no constant
    // expression.
    template<typename Operation>
    D_CONSTEXPR_CPP14
    inline_value(
        Operation,
        const dj_math_value& _x
    ) D_NOEXCEPT
        : m_value(Operation::apply(_x))
    {}

    // the same for an operation on _a and _b.
    template<typename Operation>
    D_CONSTEXPR_CPP14
    inline_value(
        Operation,
        const dj_math_value& _a,
        const dj_math_value& _b
    ) D_NOEXCEPT
        : m_value(Operation::apply(_a, _b))
    {}

public:
    // ---- construction --------------------------------------------------

    // none.
    D_CONSTEXPR_CPP14
    inline_value() D_NOEXCEPT
        : m_value(d_math_value_none())
    {}

    // a signed integer: an integer.
    D_CONSTEXPR_CPP14
    inline_value(
        int _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}

    D_CONSTEXPR_CPP14
    inline_value(
        long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    D_CONSTEXPR_CPP14
    inline_value(
        long long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}
    D_LONG_LONG_DIAG_POP
#endif

    // an unsigned integer: an integer where it fits dj_math_imax, past it
    // the nearest long double, flagged inexact (G2).
    D_CONSTEXPR_CPP14
    inline_value(
        unsigned _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}

    D_CONSTEXPR_CPP14
    inline_value(
        unsigned long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    D_CONSTEXPR_CPP14
    inline_value(
        unsigned long long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}
    D_LONG_LONG_DIAG_POP
#endif

    // a floating-point number: its own kind.
    D_CONSTEXPR_CPP14
    inline_value(
        float _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_f(_x))
    {}

    D_CONSTEXPR_CPP14
    inline_value(
        double _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_d(_x))
    {}

    D_CONSTEXPR_CPP14
    inline_value(
        long double _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_ld(_x))
    {}

    // a bool: a boolean.
    D_CONSTEXPR_CPP14
    inline_value(
        bool _truth
    ) D_NOEXCEPT
        : m_value(d_math_value_from_bool(_truth))
    {}

    // an error code: an error.
    D_CONSTEXPR_CPP14
    inline_value(
        dj_math_value_error _error
    ) D_NOEXCEPT
        : m_value(d_math_value_from_error(_error))
    {}

    // a rational of the C core's: the number, an integer where its
    // denominator is 1.
    D_CONSTEXPR_CPP14
    inline_value(
        const dj_rational& _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_rational(_x))
    {}

    // _num / _den exactly: an integer or a rational in the one form; over 0
    // a pole, or invalid for 0 / 0.
    static D_CONSTEXPR_CPP14
    inline_value ratio(
        dj_math_imax _num,
        dj_math_imax _den
    ) D_NOEXCEPT
    {
        return inline_value(d_math_value_from_ratio(_num, _den),
                            from_c_tag());
    }

    // ---- queries -------------------------------------------------------

    D_CONSTEXPR_CPP14
    dj_math_value_kind kind() const D_NOEXCEPT
    {
        return m_value.kind;
    }

    // D_MATH_VALUE_FLAG_INEXACT where an exact result did not fit (G2), or
    // came from one that did not.
    D_CONSTEXPR_CPP14
    unsigned flags() const D_NOEXCEPT
    {
        return m_value.flags;
    }

    // the place in the tower; 0 for none, an error or a boolean.
    D_CONSTEXPR_CPP14
    int rank() const D_NOEXCEPT
    {
        return d_math_value_rank(m_value);
    }

    D_CONSTEXPR_CPP14
    const char* kind_name() const D_NOEXCEPT
    {
        return d_math_value_kind_name(m_value);
    }

    D_CONSTEXPR_CPP14
    bool is_number() const D_NOEXCEPT
    {
        return d_math_value_is_number(m_value);
    }

    D_CONSTEXPR_CPP14
    bool is_exact() const D_NOEXCEPT
    {
        return d_math_value_is_exact(m_value);
    }

    // the C core's value, lent.
    D_CONSTEXPR_CPP14
    const dj_math_value& c_value() const D_NOEXCEPT
    {
        return m_value;
    }

    // ---- conversions ---------------------------------------------------
    // true, and _out written, where the value has one of _out's type;
    // false, and _out as it was, where not.

    // a number's nearest float, double or long double.
    D_CONSTEXPR_CPP14
    bool to(
        float& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_f(&_out, m_value);
    }

    D_CONSTEXPR_CPP14
    bool to(
        double& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_d(&_out, m_value);
    }

    D_CONSTEXPR_CPP14
    bool to(
        long double& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_ld(&_out, m_value);
    }

    // an exact number's rational.
    D_CONSTEXPR_CPP14
    bool to(
        dj_rational& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_rational(&_out, m_value);
    }

    // an exact integer's dj_math_imax.
    D_CONSTEXPR_CPP14
    bool to(
        dj_math_imax& _out
    ) const D_NOEXCEPT
    {
        dj_rational exact = { 0, 1 };

        // an exact number, and whole
        if ( (!d_math_value_to_rational(&exact, m_value)) ||
             (exact.den != 1) )
        {
            return false;
        }

        _out = exact.num;

        return true;
    }

    // a boolean's truth.
    D_CONSTEXPR_CPP14
    bool to(
        bool& _out
    ) const D_NOEXCEPT
    {
        // a boolean
        if (m_value.kind != D_MATH_VALUE_BOOLEAN)
        {
            return false;
        }

        _out = (m_value.code != 0);

        return true;
    }

    // an error's code.
    D_CONSTEXPR_CPP14
    bool to(
        dj_math_value_error& _out
    ) const D_NOEXCEPT
    {
        // an error
        if (m_value.kind != D_MATH_VALUE_ERROR)
        {
            return false;
        }

        _out = static_cast<dj_math_value_error>(m_value.code);

        return true;
    }

    // ---- arithmetic ----------------------------------------------------

    D_CONSTEXPR_CPP14
    inline_value& operator+=(
        const inline_value& _other
    ) D_NOEXCEPT
    {
        m_value = d_math_value_add(m_value, _other.m_value);

        return *this;
    }

    D_CONSTEXPR_CPP14
    inline_value& operator-=(
        const inline_value& _other
    ) D_NOEXCEPT
    {
        m_value = d_math_value_sub(m_value, _other.m_value);

        return *this;
    }

    D_CONSTEXPR_CPP14
    inline_value& operator*=(
        const inline_value& _other
    ) D_NOEXCEPT
    {
        m_value = d_math_value_mul(m_value, _other.m_value);

        return *this;
    }

    D_CONSTEXPR_CPP14
    inline_value& operator/=(
        const inline_value& _other
    ) D_NOEXCEPT
    {
        m_value = d_math_value_div(m_value, _other.m_value);

        return *this;
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator+(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_add(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator-(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_sub(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator*(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_mul(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator/(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_div(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator-(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_neg(), _x.m_value);
    }

    friend D_CONSTEXPR_CPP14
    inline_value operator+(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return _x;
    }

    friend D_CONSTEXPR_CPP14
    inline_value abs(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return inline_value(internal::value_abs(), _x.m_value);
    }

    // ---- comparison ----------------------------------------------------

    // how this value stands to _other, exactly: D_MATH_VALUE_LESS, _EQUAL,
    // _GREATER, or _UNORDERED.
    D_CONSTEXPR_CPP14
    dj_math_value_order compare(
        const inline_value& _other
    ) const D_NOEXCEPT
    {
        return d_math_value_cmp(m_value, _other.m_value);
    }

    friend D_CONSTEXPR_CPP14
    bool operator==(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_EQUAL);
    }

    friend D_CONSTEXPR_CPP14
    bool operator!=(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) !=
                D_MATH_VALUE_EQUAL);
    }

    friend D_CONSTEXPR_CPP14
    bool operator<(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_LESS);
    }

    friend D_CONSTEXPR_CPP14
    bool operator>(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_GREATER);
    }

    friend D_CONSTEXPR_CPP14
    bool operator<=(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        return ( (order == D_MATH_VALUE_LESS) ||
                 (order == D_MATH_VALUE_EQUAL) );
    }

    friend D_CONSTEXPR_CPP14
    bool operator>=(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        return ( (order == D_MATH_VALUE_GREATER) ||
                 (order == D_MATH_VALUE_EQUAL) );
    }

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // the order as a partial ordering: unordered for a NaN, an error and
    // unlike kinds.
    friend D_CONSTEXPR
    std::partial_ordering operator<=>(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        // by order
        if (order == D_MATH_VALUE_LESS)
        {
            return std::partial_ordering::less;
        }

        if (order == D_MATH_VALUE_EQUAL)
        {
            return std::partial_ordering::equivalent;
        }

        if (order == D_MATH_VALUE_GREATER)
        {
            return std::partial_ordering::greater;
        }

        return std::partial_ordering::unordered;
    }
#endif

private:
    friend class value;

    // a C value of an inline kind, as it is.
    D_CONSTEXPR_CPP14
    inline_value(
        const dj_math_value& _x,
        from_c_tag
    ) D_NOEXCEPT
        : m_value(_x)
    {}

    // no pointer is a boolean.
    template<typename Type>
    D_DELETED_FN(inline_value(Type* _pointer))

    dj_math_value m_value;
};


//==============================================================================
// 2.  THE VALUE
//==============================================================================


// 2.1    value
//------------------------------------------------------------------------------
// value
//   class: a value of any kind, a boxed kind's included, holding its box as
// an int holds its number: a copy is a holder more, and a value destroyed
// or overwritten a holder fewer. A literal type from C++20, a constant
// expression there for the inline kinds.
class value
{
private:
    struct adopt_tag
    {};

    // value.h's Operation on _x, its result made in place, as
    // inline_value's are, and here for the same reason: the hold on a
    // boxed result is this value's.
    template<typename Operation>
    D_CONSTEXPR_CPP20
    value(
        Operation,
        const dj_math_value& _x
    ) D_NOEXCEPT
        : m_value(Operation::apply(_x))
    {}

    // the same for an operation on _a and _b.
    template<typename Operation>
    D_CONSTEXPR_CPP20
    value(
        Operation,
        const dj_math_value& _a,
        const dj_math_value& _b
    ) D_NOEXCEPT
        : m_value(Operation::apply(_a, _b))
    {}

public:
    // ---- construction --------------------------------------------------

    // none.
    D_CONSTEXPR_CPP20
    value() D_NOEXCEPT
        : m_value(d_math_value_none())
    {}

    // a signed integer: an integer.
    D_CONSTEXPR_CPP20
    value(
        int _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}

    D_CONSTEXPR_CPP20
    value(
        long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    D_CONSTEXPR_CPP20
    value(
        long long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_int(static_cast<dj_math_imax>(_n)))
    {}
    D_LONG_LONG_DIAG_POP
#endif

    // an unsigned integer: an integer where it fits dj_math_imax, past it
    // the nearest long double, flagged inexact (G2).
    D_CONSTEXPR_CPP20
    value(
        unsigned _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}

    D_CONSTEXPR_CPP20
    value(
        unsigned long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    D_CONSTEXPR_CPP20
    value(
        unsigned long long _n
    ) D_NOEXCEPT
        : m_value(d_math_value_from_uint(static_cast<dj_math_umax>(_n)))
    {}
    D_LONG_LONG_DIAG_POP
#endif

    // a floating-point number: its own kind.
    D_CONSTEXPR_CPP20
    value(
        float _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_f(_x))
    {}

    D_CONSTEXPR_CPP20
    value(
        double _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_d(_x))
    {}

    D_CONSTEXPR_CPP20
    value(
        long double _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_ld(_x))
    {}

    // a bool: a boolean.
    D_CONSTEXPR_CPP20
    value(
        bool _truth
    ) D_NOEXCEPT
        : m_value(d_math_value_from_bool(_truth))
    {}

    // an error code: an error.
    D_CONSTEXPR_CPP20
    value(
        dj_math_value_error _error
    ) D_NOEXCEPT
        : m_value(d_math_value_from_error(_error))
    {}

    // a rational of the C core's: the number, an integer where its
    // denominator is 1.
    D_CONSTEXPR_CPP20
    value(
        const dj_rational& _x
    ) D_NOEXCEPT
        : m_value(d_math_value_from_rational(_x))
    {}

    // an inline value: the same value.
    D_CONSTEXPR_CPP20
    value(
        const inline_value& _x
    ) D_NOEXCEPT
        : m_value(_x.m_value)
    {}

    // a C value the caller keeps: a copy, one holder more of its box.
    D_CONSTEXPR_CPP20
    explicit value(
        const dj_math_value& _x
    ) D_NOEXCEPT
        : m_value(d_math_value_retain(_x))
    {}

    // _num / _den exactly: an integer or a rational in the one form; over 0
    // a pole, or invalid for 0 / 0.
    static D_CONSTEXPR_CPP20
    value ratio(
        dj_math_imax _num,
        dj_math_imax _den
    ) D_NOEXCEPT
    {
        return value(d_math_value_from_ratio(_num, _den), adopt_tag());
    }

    // a C value whose hold is the caller's, as a result of value.h's is:
    // the hold becomes this value's, with no holder more.
    static D_CONSTEXPR_CPP20
    value adopt(
        const dj_math_value& _x
    ) D_NOEXCEPT
    {
        return value(_x, adopt_tag());
    }

    // ---- ownership -----------------------------------------------------

    // one holder more.
    D_CONSTEXPR_CPP20
    value(
        const value& _other
    ) D_NOEXCEPT
        : m_value(d_math_value_retain(_other.m_value))
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // the hold, taken: _other is left none.
    D_CONSTEXPR_CPP20
    value(
        value&& _other
    ) D_NOEXCEPT
        : m_value(_other.m_value)
    {
        _other.m_value = d_math_value_none();
    }
#endif

    // one holder fewer. Only a box is released: release does nothing more
    // for an inline kind, and GCC's constant evaluation cannot take the test
    // of a temporary's address against NULL that it starts with.
    D_CONSTEXPR_CPP20
    ~value()
    {
        // a box: one holder fewer
        if (m_value.kind == D_MATH_VALUE_BOXED)
        {
            d_math_value_release(&m_value);
        }
    }

    // _other's value, held once more; this value's, one holder fewer.
    D_CONSTEXPR_CPP20 value&
    operator=(
        const value& _other
    ) D_NOEXCEPT
    {
        value copy(_other);

        swap(copy);

        return *this;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // _other's hold, taken, and _other left none; this value's, one holder
    // fewer.
    D_CONSTEXPR_CPP20 value&
    operator=(
        value&& _other
    ) D_NOEXCEPT
    {
        value taken(static_cast<value&&>(_other));

        swap(taken);

        return *this;
    }
#endif

    D_CONSTEXPR_CPP20
    void swap(
        value& _other
    ) D_NOEXCEPT
    {
        const dj_math_value held = m_value;

        m_value        = _other.m_value;
        _other.m_value = held;

        return;
    }

    friend D_CONSTEXPR_CPP20
    void swap(
        value& _a,
        value& _b
    ) D_NOEXCEPT
    {
        _a.swap(_b);

        return;
    }

    // the C value, its hold now the caller's, to give back with
    // d_math_value_release; this value is left none.
    D_CONSTEXPR_CPP20
    dj_math_value detach() D_NOEXCEPT
    {
        const dj_math_value held = m_value;

        m_value = d_math_value_none();

        return held;
    }

    // ---- queries -------------------------------------------------------

    D_CONSTEXPR_CPP20
    dj_math_value_kind kind() const D_NOEXCEPT
    {
        return m_value.kind;
    }

    // D_MATH_VALUE_FLAG_INEXACT where an exact result did not fit (G2), or
    // came from one that did not.
    D_CONSTEXPR_CPP20
    unsigned flags() const D_NOEXCEPT
    {
        return m_value.flags;
    }

    // the place in the tower: an inline number's, a boxed kind's own; 0 for
    // none, an error or a boolean.
    D_CONSTEXPR_CPP20
    int rank() const D_NOEXCEPT
    {
        return d_math_value_rank(m_value);
    }

    // the kind's name: an inline kind's, or a boxed kind's own.
    D_CONSTEXPR_CPP20
    const char* kind_name() const D_NOEXCEPT
    {
        return d_math_value_kind_name(m_value);
    }

    D_CONSTEXPR_CPP20
    bool is_number() const D_NOEXCEPT
    {
        return d_math_value_is_number(m_value);
    }

    D_CONSTEXPR_CPP20
    bool is_exact() const D_NOEXCEPT
    {
        return d_math_value_is_exact(m_value);
    }

    // the C core's value, lent: its hold stays this value's.
    D_CONSTEXPR_CPP20
    const dj_math_value& c_value() const D_NOEXCEPT
    {
        return m_value;
    }

    // ---- conversions ---------------------------------------------------
    // true, and _out written, where the value has one of _out's type;
    // false, and _out as it was, where not. A boxed number converts as its
    // kind converts it.

    // a number's nearest float, double or long double.
    D_CONSTEXPR_CPP20
    bool to(
        float& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_f(&_out, m_value);
    }

    D_CONSTEXPR_CPP20
    bool to(
        double& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_d(&_out, m_value);
    }

    D_CONSTEXPR_CPP20
    bool to(
        long double& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_ld(&_out, m_value);
    }

    // an exact number's rational.
    D_CONSTEXPR_CPP20
    bool to(
        dj_rational& _out
    ) const D_NOEXCEPT
    {
        return d_math_value_to_rational(&_out, m_value);
    }

    // an exact integer's dj_math_imax.
    D_CONSTEXPR_CPP20
    bool to(
        dj_math_imax& _out
    ) const D_NOEXCEPT
    {
        dj_rational exact = { 0, 1 };

        // an exact number, and whole
        if ( (!d_math_value_to_rational(&exact, m_value)) ||
             (exact.den != 1) )
        {
            return false;
        }

        _out = exact.num;

        return true;
    }

    // a boolean's truth.
    D_CONSTEXPR_CPP20
    bool to(
        bool& _out
    ) const D_NOEXCEPT
    {
        // a boolean
        if (m_value.kind != D_MATH_VALUE_BOOLEAN)
        {
            return false;
        }

        _out = (m_value.code != 0);

        return true;
    }

    // an error's code.
    D_CONSTEXPR_CPP20
    bool to(
        dj_math_value_error& _out
    ) const D_NOEXCEPT
    {
        // an error
        if (m_value.kind != D_MATH_VALUE_ERROR)
        {
            return false;
        }

        _out = static_cast<dj_math_value_error>(m_value.code);

        return true;
    }

    // the same value as an inline value, where it is of an inline kind.
    D_CONSTEXPR_CPP20
    bool to(
        inline_value& _out
    ) const D_NOEXCEPT
    {
        // inline
        if (m_value.kind == D_MATH_VALUE_BOXED)
        {
            return false;
        }

        _out = inline_value(m_value, inline_value::from_c_tag());

        return true;
    }

    // ---- arithmetic ----------------------------------------------------
    // each result a value of its own, value.h's made in place; an operand is
    // only lent.

    D_CONSTEXPR_CPP20
    value& operator+=(
        const value& _other
    ) D_NOEXCEPT
    {
        value result(internal::value_add(), m_value, _other.m_value);

        swap(result);

        return *this;
    }

    D_CONSTEXPR_CPP20
    value& operator-=(
        const value& _other
    ) D_NOEXCEPT
    {
        value result(internal::value_sub(), m_value, _other.m_value);

        swap(result);

        return *this;
    }

    D_CONSTEXPR_CPP20
    value& operator*=(
        const value& _other
    ) D_NOEXCEPT
    {
        value result(internal::value_mul(), m_value, _other.m_value);

        swap(result);

        return *this;
    }

    D_CONSTEXPR_CPP20
    value& operator/=(
        const value& _other
    ) D_NOEXCEPT
    {
        value result(internal::value_div(), m_value, _other.m_value);

        swap(result);

        return *this;
    }

    friend D_CONSTEXPR_CPP20
    value operator+(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return value(internal::value_add(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP20
    value operator-(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return value(internal::value_sub(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP20
    value operator*(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return value(internal::value_mul(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP20
    value operator/(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return value(internal::value_div(), _a.m_value, _b.m_value);
    }

    friend D_CONSTEXPR_CPP20
    value operator-(
        const value& _x
    ) D_NOEXCEPT
    {
        return value(internal::value_neg(), _x.m_value);
    }

    friend D_CONSTEXPR_CPP20
    value operator+(
        const value& _x
    ) D_NOEXCEPT
    {
        return _x;
    }

    friend D_CONSTEXPR_CPP20
    value abs(
        const value& _x
    ) D_NOEXCEPT
    {
        return value(internal::value_abs(), _x.m_value);
    }

    // ---- comparison ----------------------------------------------------

    // how this value stands to _other, exactly: D_MATH_VALUE_LESS, _EQUAL,
    // _GREATER, or _UNORDERED.
    D_CONSTEXPR_CPP20
    dj_math_value_order compare(
        const value& _other
    ) const D_NOEXCEPT
    {
        return d_math_value_cmp(m_value, _other.m_value);
    }

    friend D_CONSTEXPR_CPP20
    bool operator==(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_EQUAL);
    }

    friend D_CONSTEXPR_CPP20
    bool operator!=(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) !=
                D_MATH_VALUE_EQUAL);
    }

    friend D_CONSTEXPR_CPP20
    bool operator<(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_LESS);
    }

    friend D_CONSTEXPR_CPP20
    bool operator>(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return (d_math_value_cmp(_a.m_value, _b.m_value) ==
                D_MATH_VALUE_GREATER);
    }

    friend D_CONSTEXPR_CPP20
    bool operator<=(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        return ( (order == D_MATH_VALUE_LESS) ||
                 (order == D_MATH_VALUE_EQUAL) );
    }

    friend D_CONSTEXPR_CPP20
    bool operator>=(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        return ( (order == D_MATH_VALUE_GREATER) ||
                 (order == D_MATH_VALUE_EQUAL) );
    }

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // the order as a partial ordering: unordered for a NaN, an error and
    // unlike kinds.
    friend D_CONSTEXPR
    std::partial_ordering operator<=>(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        const dj_math_value_order order = d_math_value_cmp(_a.m_value,
                                                          _b.m_value);

        // by order
        if (order == D_MATH_VALUE_LESS)
        {
            return std::partial_ordering::less;
        }

        if (order == D_MATH_VALUE_EQUAL)
        {
            return std::partial_ordering::equivalent;
        }

        if (order == D_MATH_VALUE_GREATER)
        {
            return std::partial_ordering::greater;
        }

        return std::partial_ordering::unordered;
    }
#endif

private:
    // a C value whose hold this value takes, as it is.
    D_CONSTEXPR_CPP20
    value(
        const dj_math_value& _x,
        adopt_tag
    ) D_NOEXCEPT
        : m_value(_x)
    {}

    // no pointer is a boolean.
    template<typename Type>
    D_DELETED_FN(value(Type* _pointer))

    dj_math_value m_value;
};


//==============================================================================
// 3.  NUMBERS
//==============================================================================
// The two classes as numbers for generic code (number_traits.hpp): their
// traits, and the type each makes with the other and with the arithmetic
// types. The concept number is number_traits.hpp's; both classes satisfy it.


// 3.1    The square root
//------------------------------------------------------------------------------
NS_INTERNAL

    // value_whole_root
    //   function: whether _n is a perfect square, and if so its root, in
    // _root: Newton's iteration on the integers, from above, exact; false
    // for a negative _n.
    inline D_CONSTEXPR_CPP14
    bool value_whole_root(
        dj_math_imax  _n,
        dj_math_imax& _root
    ) D_NOEXCEPT
    {
        dj_math_umax n    = 0u;
        dj_math_umax x    = 0u;
        dj_math_umax next = 0u;

        if (_n < 0)
        {
            return false;
        }

        n = static_cast<dj_math_umax>(_n);

        // 0 and 1 are their own roots
        if (n < 2u)
        {
            _root = _n;

            return true;
        }

        // n / 2 + 1 is above the root of any n >= 2; each step comes down,
        // until the root's floor
        x    = (n / 2u) + 1u;
        next = (x + (n / x)) / 2u;

        while (next < x)
        {
            x    = next;
            next = (x + (n / x)) / 2u;
        }

        if ((x * x) != n)
        {
            return false;
        }

        _root = static_cast<dj_math_imax>(x);

        return true;
    }

    // value_root_of
    //   function: the square root of _x, for either class. Where _x is an
    // exact rational of the family whose terms are perfect squares, the
    // exact root, so the norm of (3, 4) is 5; an exact negative number, a
    // domain error; any other number, the long double nearest the root of
    // the long double nearest _x, a NaN for a negative one, as for the
    // floating-point types. An error is its own root; what is no number has
    // none: a domain error.
    template<typename Value>
    D_CONSTEXPR_CPP14
    Value value_root_of(
        const Value& _x
    ) D_NOEXCEPT
    {
        dj_rational  exact    = { 0, 1 };
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;
        long double nearest  = 0.0L;

        // an error passes; what is no number has no root
        if (_x.kind() == D_MATH_VALUE_ERROR)
        {
            return _x;
        }

        if (!_x.is_number())
        {
            return Value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // exact: no root for a negative number, a wide integer past the
        // family among them; the exact root of a rational of the family
        // whose terms are squares
        if (_x.is_exact())
        {
            if (_x < Value(0))
            {
                return Value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            if ( (_x.to(exact)) &&
                 (value_whole_root(exact.num, num_root)) &&
                 (value_whole_root(exact.den, den_root)) )
            {
                return Value::ratio(num_root, den_root);
            }
        }

        (void)_x.to(nearest);

        return Value(number_root(nearest));
    }

    // value_magnitude
    //   function: |_x|, the class's own abs, which argument-dependent lookup
    // finds; number_traits' member of that name would hide it.
    template<typename Value>
    D_CONSTEXPR_CPP14
    Value value_magnitude(
        const Value& _x
    ) D_NOEXCEPT
    {
        return abs(_x);
    }

    // value_epsilon
    //   function: the precision _x carries, as a value of its own kind: 0 for
    // an exact one; a float's, a double's or a long double's epsilon for one
    // of those kinds; a long double's for any other inexact kind, G2's
    // (S158).
    template<typename Value>
    D_CONSTEXPR_CPP14
    Value value_epsilon(
        const Value& _x
    ) D_NOEXCEPT
    {
        // exact: no rounding to carry
        if (_x.is_exact())
        {
            return Value(0);
        }

        // by kind
        switch (_x.kind())
        {
            case D_MATH_VALUE_FLOAT:
                return Value(FLT_EPSILON);

            case D_MATH_VALUE_DOUBLE:
                return Value(DBL_EPSILON);

            default:
                break;
        }

        return Value(LDBL_EPSILON);
    }

    // value_tolerance
    //   function: the tolerance for comparing _a and _b: 0 where both are
    // exact (S158), else four of the larger epsilon times the larger
    // magnitude (S157).
    template<typename Value>
    D_CONSTEXPR_CPP14
    Value value_tolerance(
        const Value& _a,
        const Value& _b
    ) D_NOEXCEPT
    {
        // both exact: no tolerance
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            return Value(0);
        }

        {
            const Value ea = value_epsilon(_a);
            const Value eb = value_epsilon(_b);
            const Value ma = value_magnitude(_a);
            const Value mb = value_magnitude(_b);

            return (Value(4) * ((ea < eb) ? eb : ea)) * ((ma < mb) ? mb : ma);
        }
    }

    // value_to_double
    //   function: the double nearest _x, and a NaN where _x is no number.
    template<typename Value>
    D_CONSTEXPR_CPP14
    double value_to_double(
        const Value& _x
    ) D_NOEXCEPT
    {
        double nearest = 0.0;

        return (_x.to(nearest)) ? nearest
                                : std::numeric_limits<double>::quiet_NaN();
    }

    // value_common_number
    //   trait: Result where Other is an arithmetic type, nothing otherwise.
    template<typename Result,
             typename Other,
             bool     Arithmetic = (arithmetic_number<Other>::value != 0)>
    struct value_common_number
    {};

    template<typename Result,
             typename Other>
    struct value_common_number<Result, Other, true>
    {
        typedef Result type;
    };

NS_END  // internal

// 3.2    number_traits
//------------------------------------------------------------------------------
// number_traits<inline_value>
//   trait: an inline value as a number: not exact, as it may hold a float
// or G2's long double, but a field, commutative and ordered; its zero and
// one the integers; |x|, its magnitude, and the conversions value.h's; its
// root exact where the value is the square of a rational of the family, and
// otherwise the long double nearest it; the precision a value carries 0
// where it is exact and its kind's epsilon where not; and for a tolerance
// four epsilons, of a double's where no operands are given and of the
// larger of two operands' times their larger magnitude where they are, 0
// where both are exact (S157, S158). Constant expressions from C++14.
template<>
struct number_traits<inline_value>
{
    typedef inline_value magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = false;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = true;
    static const bool is_ordered     = true;

    static D_CONSTEXPR_CPP14
    inline_value zero() D_NOEXCEPT
    {
        return inline_value(0);
    }

    static D_CONSTEXPR_CPP14
    inline_value one() D_NOEXCEPT
    {
        return inline_value(1);
    }

    static D_CONSTEXPR_CPP14
    inline_value abs(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return internal::value_magnitude(_x);
    }

    static D_CONSTEXPR_CPP14
    inline_value magnitude(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return internal::value_magnitude(_x);
    }

    static D_CONSTEXPR_CPP14
    double to_double(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return internal::value_to_double(_x);
    }

    static D_CONSTEXPR_CPP14
    inline_value from_double(
        double _x
    ) D_NOEXCEPT
    {
        return inline_value(_x);
    }

    static D_CONSTEXPR_CPP14
    inline_value sqrt(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return internal::value_root_of(_x);
    }

    static D_CONSTEXPR_CPP14
    inline_value epsilon() D_NOEXCEPT
    {
        return inline_value(DBL_EPSILON);
    }

    static D_CONSTEXPR_CPP14
    inline_value epsilon(
        const inline_value& _x
    ) D_NOEXCEPT
    {
        return internal::value_epsilon(_x);
    }

    static D_CONSTEXPR_CPP14
    inline_value tolerance() D_NOEXCEPT
    {
        return inline_value(4.0 * DBL_EPSILON);
    }

    static D_CONSTEXPR_CPP14
    inline_value tolerance(
        const inline_value& _a,
        const inline_value& _b
    ) D_NOEXCEPT
    {
        return internal::value_tolerance(_a,
                                         _b);
    }
};

// number_traits<value>
//   trait: a value as a number, as an inline value is; constant expressions
// from C++20.
template<>
struct number_traits<value>
{
    typedef value magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = false;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = true;
    static const bool is_ordered     = true;

    static D_CONSTEXPR_CPP20
    value zero() D_NOEXCEPT
    {
        return value(0);
    }

    static D_CONSTEXPR_CPP20
    value one() D_NOEXCEPT
    {
        return value(1);
    }

    static D_CONSTEXPR_CPP20
    value abs(
        const value& _x
    ) D_NOEXCEPT
    {
        return internal::value_magnitude(_x);
    }

    static D_CONSTEXPR_CPP20
    value magnitude(
        const value& _x
    ) D_NOEXCEPT
    {
        return internal::value_magnitude(_x);
    }

    static D_CONSTEXPR_CPP20
    double to_double(
        const value& _x
    ) D_NOEXCEPT
    {
        return internal::value_to_double(_x);
    }

    static D_CONSTEXPR_CPP20
    value from_double(
        double _x
    ) D_NOEXCEPT
    {
        return value(_x);
    }

    static D_CONSTEXPR_CPP20
    value sqrt(
        const value& _x
    ) D_NOEXCEPT
    {
        return internal::value_root_of(_x);
    }

    static D_CONSTEXPR_CPP20
    value epsilon() D_NOEXCEPT
    {
        return value(DBL_EPSILON);
    }

    static D_CONSTEXPR_CPP20
    value epsilon(
        const value& _x
    ) D_NOEXCEPT
    {
        return internal::value_epsilon(_x);
    }

    static D_CONSTEXPR_CPP20
    value tolerance() D_NOEXCEPT
    {
        return value(4.0 * DBL_EPSILON);
    }

    static D_CONSTEXPR_CPP20
    value tolerance(
        const value& _a,
        const value& _b
    ) D_NOEXCEPT
    {
        return internal::value_tolerance(_a,
                                         _b);
    }
};

// 3.3    common_number
//------------------------------------------------------------------------------
// An inline value with an inline value or an arithmetic type stays inline;
// with a value, or a value with anything numeric, is a value: the classes'
// operators' own types, at every level.

template<>
struct common_number<inline_value, inline_value>
{
    typedef inline_value type;
};

template<>
struct common_number<value, value>
{
    typedef value type;
};

template<>
struct common_number<inline_value, value>
{
    typedef value type;
};

template<>
struct common_number<value, inline_value>
{
    typedef value type;
};

template<typename Other>
struct common_number<inline_value, Other>
    : internal::value_common_number<inline_value, Other>
{};

template<typename Other>
struct common_number<Other, inline_value>
    : internal::value_common_number<inline_value, Other>
{};

template<typename Other>
struct common_number<value, Other>
    : internal::value_common_number<value, Other>
{};

template<typename Other>
struct common_number<Other, value>
    : internal::value_common_number<value, Other>
{};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_VALUE_HPP
