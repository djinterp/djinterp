/*******************************************************************************
* djinterp [cpp/math]                                   t_number_traits_face.cpp
*
*   Tests for math/number_traits.hpp at every level from C++98, and for the
* linear-algebra subframework over number types the C core does not hold:
*   - number_traits for each arithmetic type: its flags, the field's and the
*     order's, zero and one, |x| and the magnitude at the type's ends, the
*     root (the C library's, correctly rounded, for a floating-point type;
*     exact, rounded down, for an integer, S157), the conversions through
*     double, the epsilon, and the tolerance, four epsilons relative to the
*     larger magnitude (0 for an integer);
*   - the primary template, over t_real, a user's type that converts to and
*     from double, as the generic path always took one;
*   - number_traits for math::inline_value and math::value: zero and one,
*     |x|, the conversions, and the root, exact for the square of a rational
*     of the family, a domain error for an exact negative number, and the
*     long double's otherwise; the epsilon a value carries, by kind, and a
*     pair's tolerance, 0 where both are exact (S158);
*   - common_number for every pair of arithmetic types, against the type
*     a + b has, and for the value classes with each other and with every
*     arithmetic type, against their operators' types;
*   - vector and matrix over math::value: zero by default; exact sums, dot
*     and cross products, norms, and unit vectors where they are exact; the
*     Hilbert matrix's exact determinant and inverse, its product with the
*     inverse the identity, and its solution by elimination exactly the
*     inverse's times B (S257); inexact values solved and inverted with
*     each pivot the entry of largest magnitude; wide integers past
*     dj_math_imax, every box given back;
*   - the same over t_ratio, an exact rational type of the test's own,
*     which joins by specializing number_traits, as a bignum would, a
*     comparison given no tolerance taking its author's;
*   - matrices over t_quat, an exact quaternion of the test's own, whose
*     products do not commute: the inverse on both sides and the solution
*     exact, each product in its order (S269);
*   - from C++14, vectors and matrices of inline values as constant
*     expressions; from C++20, of values too, and the concept number.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_number_traits_face.cpp             \
*         -o t_number_traits -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_number_traits_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/number_traits.hpp"  // tested
#include "../../../../../inc/djinterp/cpp/modules/math/value.hpp"  // its value classes
#include "../../../../../inc/djinterp/cpp/modules/math/linear_algebra/square.hpp"  // determinant,
                                                                                   // inverse
#include "../../../../../inc/djinterp/cpp/modules/math/linear_algebra/solve.hpp"  // solve
// std
#include <cfloat>   // FLT_EPSILON, DBL_EPSILON, LDBL_EPSILON
#include <climits>  // INT_MAX, INT_MIN, LONG_MAX, UINT_MAX, ...
#include <cmath>    // std::sqrt
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // std::malloc, std::free
#include <cstring>  // std::strlen
#include <limits>   // std::numeric_limits

// the wide integer's boxes come from the test, which counts them: its two
// knobs, set before value_wide.h is first included
void* d_tests_allocate(std::size_t _size);
void  d_tests_give_back(void* _box);
#define D_CFG_MATH_VALUE_WIDE_MALLOC(_size) d_tests_allocate(_size)
#define D_CFG_MATH_VALUE_WIDE_FREE(_ptr)    d_tests_give_back(_ptr)
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/value_wide.h"  // a boxed kind


namespace
{
    namespace dm = djinterp::math;
    namespace la = djinterp::math::linalg;

    long g_checks   = 0;
    long g_failures = 0;
    long g_live     = 0;
    long g_made     = 0;

#if D_ENV_HAS_LONG_LONG
    D_LONG_LONG_DIAG_PUSH
    typedef long long          t_llong;
    typedef unsigned long long t_ullong;
    D_LONG_LONG_DIAG_POP
#endif
}  // namespace

// d_tests_allocate
//   a box's storage, counted.
void*
d_tests_allocate(
    std::size_t _size
)
{
    void* box = std::malloc(_size);

    // counted where given
    if (box != D_NULLPTR)
    {
        ++g_live;
        ++g_made;
    }

    return box;
}

// d_tests_give_back
//   a box given back, counted.
void
d_tests_give_back(
    void* _box
)
{
    // counted where there was one
    if (_box != D_NULLPTR)
    {
        --g_live;
    }

    std::free(_box);

    return;
}


namespace
{
    //==========================================================================
    // 1.  REPORTING
    //==========================================================================

    // d_tests_expect
    //   one check, reported where it fails.
    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures with what was checked, and where
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s\n", _line, _what);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what)                                          \
        d_tests_expect((ok), (what), __LINE__)

    // d_tests_same_real
    //   two floating-point numbers the same once stored: equal, or both NaN.
    template<typename Real>
    bool
    d_tests_same_real(
        Real _a,
        Real _b
    )
    {
        volatile Real a = _a;
        volatile Real b = _b;

        return ( (a == b) ||
                 ( (a != a) &&
                   (b != b) ) );
    }

    // d_tests_is
    //   a value of _kind equal to _want: the kind, and value.h's order.
    template<typename Value>
    bool
    d_tests_is(
        const Value&     _x,
        dj_math_value_kind _kind,
        const Value&     _want
    )
    {
        return ( (_x.kind() == _kind) &&
                 (_x.compare(_want) == D_MATH_VALUE_EQUAL) );
    }


    //==========================================================================
    // 2.  TYPES
    //==========================================================================

    // d_tests_same_type
    //   1 where A and B are the one type.
    template<typename A,
             typename B>
    struct d_tests_same_type
    {
        static const bool value = false;
    };

    template<typename A>
    struct d_tests_same_type<A, A>
    {
        static const bool value = true;
    };

    // d_tests_type_of
    //   a number for each type a + b can have: told by overloading, so at
    // every level, with no decltype.
    int d_tests_type_of(int) { return 1; }
    int d_tests_type_of(unsigned) { return 2; }
    int d_tests_type_of(long) { return 3; }
    int d_tests_type_of(unsigned long) { return 4; }
#if D_ENV_HAS_LONG_LONG
    int d_tests_type_of(t_llong) { return 5; }
    int d_tests_type_of(t_ullong) { return 6; }
#endif
    int d_tests_type_of(float) { return 7; }
    int d_tests_type_of(double) { return 8; }
    int d_tests_type_of(long double) { return 9; }
    int d_tests_type_of(const dm::inline_value&) { return 10; }
    int d_tests_type_of(const dm::value&) { return 11; }

    // t_real
    //   a user's numeric type, wrapping a double: no traits of its own, so
    // the primary template serves it, as the generic path always took one.
    struct t_real
    {
        double v;

        t_real() : v(0.0) {}
        t_real(double _v) : v(_v) {}

        operator double() const { return v; }

        t_real& operator+=(const t_real& _o) { v += _o.v; return *this; }
        t_real& operator*=(const t_real& _o) { v *= _o.v; return *this; }

        friend t_real operator+(t_real _a, t_real _b)
        {
            return t_real(_a.v + _b.v);
        }

        friend t_real operator-(t_real _a, t_real _b)
        {
            return t_real(_a.v - _b.v);
        }

        friend t_real operator*(t_real _a, t_real _b)
        {
            return t_real(_a.v * _b.v);
        }

        friend t_real operator/(t_real _a, t_real _b)
        {
            return t_real(_a.v / _b.v);
        }

        friend t_real operator-(t_real _a) { return t_real(-_a.v); }
        friend bool operator<(t_real _a, t_real _b) { return _a.v < _b.v; }
        friend bool operator==(t_real _a, t_real _b) { return _a.v == _b.v; }
    };

    // t_ratio
    //   an exact rational of the test's own, over long, reduced, its
    // denominator positive: not arithmetic, held by no C kernel, and with no
    // conversion to double. It joins generic code by specializing
    // number_traits, below, as a bignum would.
    struct t_ratio
    {
        long num;
        long den;

        t_ratio()
            : num(0),
              den(1)
        {}

        t_ratio(
            int _n
        )
            : num(_n),
              den(1)
        {}

        t_ratio(
            long _n,
            long _d
        )
            : num(_n),
              den(_d)
        {
            long a = (num < 0) ? -num : num;
            long b = (den < 0) ? -den : den;

            // the gcd, and the sign onto the numerator
            while (b != 0)
            {
                const long t = a % b;

                a = b;
                b = t;
            }

            if (a > 1)
            {
                num = num / a;
                den = den / a;
            }

            if (den < 0)
            {
                num = -num;
                den = -den;
            }
        }

        t_ratio& operator+=(const t_ratio& _o)
        {
            *this = t_ratio((num * _o.den) + (_o.num * den), den * _o.den);

            return *this;
        }

        t_ratio& operator*=(const t_ratio& _o)
        {
            *this = t_ratio(num * _o.num, den * _o.den);

            return *this;
        }

        friend t_ratio operator+(t_ratio _a, const t_ratio& _b)
        {
            return _a += _b;
        }

        friend t_ratio operator*(t_ratio _a, const t_ratio& _b)
        {
            return _a *= _b;
        }

        friend t_ratio operator-(const t_ratio& _a)
        {
            return t_ratio(-_a.num, _a.den);
        }

        friend t_ratio operator-(const t_ratio& _a, const t_ratio& _b)
        {
            return _a + (-_b);
        }

        friend t_ratio operator/(const t_ratio& _a, const t_ratio& _b)
        {
            return t_ratio(_a.num * _b.den, _a.den * _b.num);
        }

        friend bool operator==(const t_ratio& _a, const t_ratio& _b)
        {
            return ( (_a.num == _b.num) &&
                     (_a.den == _b.den) );
        }

        friend bool operator<(const t_ratio& _a, const t_ratio& _b)
        {
            return ((_a.num * _b.den) < (_b.num * _a.den));
        }

        friend bool operator>(const t_ratio& _a, const t_ratio& _b)
        {
            return (_b < _a);
        }
    };

    // t_quat
    //   an exact quaternion of the test's own, w + x i + y j + z k over
    // t_ratio: a division ring, its products not commuting (i j = k, j i =
    // -k), with no order. It joins generic code by specializing
    // number_traits, its magnitude the squared norm, a nonnegative t_ratio:
    // the generic kernels must keep each product in its order (S269).
    struct t_quat
    {
        t_ratio w;
        t_ratio x;
        t_ratio y;
        t_ratio z;

        t_quat()
            : w(),
              x(),
              y(),
              z()
        {}

        t_quat(
            int _w
        )
            : w(_w),
              x(),
              y(),
              z()
        {}

        t_quat(
            const t_ratio& _w,
            const t_ratio& _x,
            const t_ratio& _y,
            const t_ratio& _z
        )
            : w(_w),
              x(_x),
              y(_y),
              z(_z)
        {}

        // the squared norm, w^2 + x^2 + y^2 + z^2
        t_ratio norm_squared() const
        {
            return (w * w) + (x * x) + (y * y) + (z * z);
        }

        t_quat& operator+=(const t_quat& _o)
        {
            w = w + _o.w;
            x = x + _o.x;
            y = y + _o.y;
            z = z + _o.z;

            return *this;
        }

        friend t_quat operator+(t_quat _a, const t_quat& _b)
        {
            return _a += _b;
        }

        friend t_quat operator-(const t_quat& _a)
        {
            return t_quat(-_a.w, -_a.x, -_a.y, -_a.z);
        }

        friend t_quat operator-(const t_quat& _a, const t_quat& _b)
        {
            return _a + (-_b);
        }

        // Hamilton's product, _a on the left
        friend t_quat operator*(const t_quat& _a, const t_quat& _b)
        {
            return t_quat(
                (_a.w * _b.w) - (_a.x * _b.x) - (_a.y * _b.y) - (_a.z * _b.z),
                (_a.w * _b.x) + (_a.x * _b.w) + (_a.y * _b.z) - (_a.z * _b.y),
                (_a.w * _b.y) - (_a.x * _b.z) + (_a.y * _b.w) + (_a.z * _b.x),
                (_a.w * _b.z) + (_a.x * _b.y) - (_a.y * _b.x) + (_a.z * _b.w));
        }

        // _a _b^-1, _b^-1 its conjugate over its squared norm; the kernels
        // divide only 1 by a pivot where products do not commute
        friend t_quat operator/(const t_quat& _a, const t_quat& _b)
        {
            const t_ratio n = _b.norm_squared();

            return _a * t_quat(_b.w / n, -_b.x / n, -_b.y / n, -_b.z / n);
        }

        friend bool operator==(const t_quat& _a, const t_quat& _b)
        {
            return ( (_a.w == _b.w) &&
                     (_a.x == _b.x) &&
                     (_a.y == _b.y) &&
                     (_a.z == _b.z) );
        }
    };
}  // namespace


NS_DJINTERP
NS_MATH

// number_traits<t_ratio>
//   trait: the test's rational as a number: exact, a field, commutative and
// ordered, its magnitude |x|; its root exact where both terms are squares,
// and through double otherwise, to the nearest sixty-five-thousandth; its
// epsilon 0, and its tolerance a thousandth, its author's choice, which a
// comparison given none must take (tolerance(a, b)).
template<>
struct number_traits<t_ratio>
{
    typedef t_ratio magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = true;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = true;
    static const bool is_ordered     = true;

    static t_ratio
    zero()
    {
        return t_ratio(0);
    }

    static t_ratio
    one()
    {
        return t_ratio(1);
    }

    static t_ratio
    abs(
        const t_ratio& _x
    )
    {
        return (_x.num < 0) ? -_x : _x;
    }

    static double
    to_double(
        const t_ratio& _x
    )
    {
        return static_cast<double>(_x.num) / static_cast<double>(_x.den);
    }

    static t_ratio
    from_double(
        double _x
    )
    {
        const double scaled = _x * 65536.0;

        return t_ratio(static_cast<long>((scaled < 0.0) ? (scaled - 0.5)
                                                        : (scaled + 0.5)),
                       65536L);
    }

    static t_ratio
    sqrt(
        const t_ratio& _x
    )
    {
        const long n =
            static_cast<long>(std::sqrt(static_cast<double>(_x.num)));
        const long d =
            static_cast<long>(std::sqrt(static_cast<double>(_x.den)));

        // the squares' roots, exactly
        if ( (_x.num >= 0) &&
             ((n * n) == _x.num) &&
             ((d * d) == _x.den) )
        {
            return t_ratio(n, d);
        }

        return from_double(std::sqrt(to_double(_x)));
    }

    static t_ratio
    magnitude(
        const t_ratio& _x
    )
    {
        return abs(_x);
    }

    static t_ratio
    epsilon()
    {
        return t_ratio(0);
    }

    static t_ratio
    epsilon(
        const t_ratio& _x
    )
    {
        return (static_cast<void>(_x), t_ratio(0));
    }

    static t_ratio
    tolerance()
    {
        return t_ratio(1L, 1000L);
    }

    static t_ratio
    tolerance(
        const t_ratio& _a,
        const t_ratio& _b
    )
    {
        return (static_cast<void>(_a), static_cast<void>(_b),
                t_ratio(1L, 1000L));
    }
};

// number_traits<t_quat>
//   trait: the test's quaternion as a number: exact, a division ring whose
// products do not commute, with no order; its magnitude the squared norm, a
// t_ratio; its epsilon and tolerance 0, of the magnitude's type.
template<>
struct number_traits<t_quat>
{
    typedef t_ratio magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = true;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = false;
    static const bool is_ordered     = false;

    static t_quat
    zero()
    {
        return t_quat(0);
    }

    static t_quat
    one()
    {
        return t_quat(1);
    }

    static t_ratio
    magnitude(
        const t_quat& _x
    )
    {
        return _x.norm_squared();
    }

    static t_ratio
    epsilon()
    {
        return t_ratio(0);
    }

    static t_ratio
    epsilon(
        const t_quat& _x
    )
    {
        return (static_cast<void>(_x), t_ratio(0));
    }

    static t_ratio
    tolerance()
    {
        return t_ratio(0);
    }

    static t_ratio
    tolerance(
        const t_quat& _a,
        const t_quat& _b
    )
    {
        return (static_cast<void>(_a), static_cast<void>(_b), t_ratio(0));
    }
};

NS_END  // math
NS_END  // djinterp


namespace
{
    //==========================================================================
    // 3.  NUMBER_TRAITS OF THE ARITHMETIC TYPES
    //==========================================================================

    // d_tests_integer_traits
    //   an integer type's traits, at its ends.
    template<typename Type>
    void
    d_tests_integer_traits(
        const char* _name,
        bool        _signed
    )
    {
        typedef dm::number_traits<Type> traits;

        const Type most  = std::numeric_limits<Type>::max();
        const Type least = std::numeric_limits<Type>::min();
        const Type roots[] = { static_cast<Type>(0), static_cast<Type>(1),
                               static_cast<Type>(2), static_cast<Type>(3),
                               static_cast<Type>(4), static_cast<Type>(15),
                               static_cast<Type>(16), static_cast<Type>(17),
                               static_cast<Type>(99), static_cast<Type>(100),
                               static_cast<Type>(most - 1), most };
        bool roots_ok = true;

        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (traits::is_exact) &&
                        (traits::is_integer) &&
                        (!traits::is_field) &&
                        (traits::is_commutative) &&
                        (traits::is_ordered),
                        _name);
        D_TESTS_EXPECT( (traits::zero() == static_cast<Type>(0)) &&
                        (traits::one() == static_cast<Type>(1)) &&
                        (traits::tolerance() == static_cast<Type>(0)) &&
                        (traits::epsilon() == static_cast<Type>(0)) &&
                        (traits::epsilon(most) == static_cast<Type>(0)) &&
                        (traits::tolerance(most, static_cast<Type>(1)) ==
                         static_cast<Type>(0)) &&
                        (traits::magnitude(most) == most),
                        _name);
        D_TESTS_EXPECT( (traits::abs(most) == most) &&
                        (traits::abs(static_cast<Type>(0)) ==
                         static_cast<Type>(0)) &&
                        ( (!_signed) ||
                          ( (traits::abs(static_cast<Type>(least + 1)) ==
                             most) &&
                            (traits::abs(static_cast<Type>(-5)) ==
                             static_cast<Type>(5)) ) ),
                        _name);
        D_TESTS_EXPECT( (traits::to_double(static_cast<Type>(7)) == 7.0) &&
                        (traits::from_double(2.75) == static_cast<Type>(2)),
                        _name);

        // the root: exact, rounded down (S157): r r <= x < (r + 1)^2, told
        // by quotients, which no product overflows; a negative number's 0
        for (std::size_t i = 0; i < (sizeof(roots) / sizeof(roots[0])); ++i)
        {
            const Type x = roots[i];
            const Type r = traits::sqrt(x);
            const Type q = static_cast<Type>(r + 1);

            roots_ok = ( (roots_ok) &&
                         ( (x < static_cast<Type>(2))
                               ? (r == x)
                               : ( (r <= static_cast<Type>(x / r)) &&
                                   (q > static_cast<Type>(x / q)) ) ) );
        }

        D_TESTS_EXPECT( (roots_ok) &&
                        ( (!_signed) ||
                          (traits::sqrt(static_cast<Type>(-4)) ==
                           static_cast<Type>(0)) ),
                        _name);

        return;
    }

    // d_tests_floating_traits
    //   a floating-point type's traits; its root the C library's.
    template<typename Type>
    void
    d_tests_floating_traits(
        const char* _name
    )
    {
        typedef dm::number_traits<Type> traits;

        const Type values[] = { static_cast<Type>(0), static_cast<Type>(1),
                                static_cast<Type>(2), static_cast<Type>(0.5),
                                static_cast<Type>(3), static_cast<Type>(1e10),
                                static_cast<Type>(1e-10),
                                static_cast<Type>(12345.678),
                                std::numeric_limits<Type>::max(),
                                std::numeric_limits<Type>::denorm_min(),
                                std::numeric_limits<Type>::infinity() };
        bool roots_ok = true;

        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (!traits::is_exact) &&
                        (!traits::is_integer) &&
                        (traits::is_field) &&
                        (traits::is_commutative) &&
                        (traits::is_ordered),
                        _name);

        // four epsilons, relative (S157): times the larger magnitude
        D_TESTS_EXPECT( (traits::zero() == static_cast<Type>(0)) &&
                        (traits::one() == static_cast<Type>(1)) &&
                        (d_tests_same_real(traits::epsilon(),
                                           std::numeric_limits<Type>::
                                               epsilon())) &&
                        (d_tests_same_real(traits::epsilon(
                                               static_cast<Type>(1e10)),
                                           std::numeric_limits<Type>::
                                               epsilon())) &&
                        (d_tests_same_real(traits::tolerance(),
                                           static_cast<Type>(4) *
                                           std::numeric_limits<Type>::
                                               epsilon())) &&
                        (d_tests_same_real(
                            traits::tolerance(static_cast<Type>(-8),
                                              static_cast<Type>(2)),
                            static_cast<Type>(32) *
                            std::numeric_limits<Type>::epsilon())) &&
                        (d_tests_same_real(traits::magnitude(
                                               static_cast<Type>(-2.5)),
                                           static_cast<Type>(2.5))),
                        _name);
        D_TESTS_EXPECT( (traits::abs(static_cast<Type>(-2.5)) ==
                         static_cast<Type>(2.5)) &&
                        (traits::abs(static_cast<Type>(2.5)) ==
                         static_cast<Type>(2.5)) &&
                        (traits::to_double(static_cast<Type>(0.5)) == 0.5) &&
                        (traits::from_double(0.25) == static_cast<Type>(0.25)),
                        _name);

        // the root: correctly rounded, as the C library's is; of a negative
        // number, a NaN
        for (std::size_t i = 0; i < (sizeof(values) / sizeof(values[0])); ++i)
        {
            roots_ok = ( (roots_ok) &&
                         (d_tests_same_real(traits::sqrt(values[i]),
                                            static_cast<Type>(
                                                std::sqrt(values[i])))) );
        }

        D_TESTS_EXPECT( (roots_ok) &&
                        (traits::sqrt(static_cast<Type>(-1)) !=
                         traits::sqrt(static_cast<Type>(-1))),
                        _name);

        return;
    }

    // d_tests_arithmetic_traits
    //   every arithmetic type's traits.
    void
    d_tests_arithmetic_traits()
    {
        d_tests_integer_traits<signed char>("signed char's traits", true);
        d_tests_integer_traits<unsigned char>("unsigned char's traits",
                                              false);
        d_tests_integer_traits<short>("short's traits", true);
        d_tests_integer_traits<unsigned short>("unsigned short's traits",
                                               false);
        d_tests_integer_traits<int>("int's traits", true);
        d_tests_integer_traits<unsigned>("unsigned's traits", false);
        d_tests_integer_traits<long>("long's traits", true);
        d_tests_integer_traits<unsigned long>("unsigned long's traits", false);
#if D_ENV_HAS_LONG_LONG
        d_tests_integer_traits<t_llong>("long long's traits", true);
        d_tests_integer_traits<t_ullong>("unsigned long long's traits", false);
#endif
        d_tests_floating_traits<float>("float's traits");
        d_tests_floating_traits<double>("double's traits");
        d_tests_floating_traits<long double>("long double's traits");

        return;
    }

    // d_tests_primary_traits
    //   the primary template, over t_real: made from an int, through double.
    void
    d_tests_primary_traits()
    {
        typedef dm::number_traits<t_real> traits;

        D_TESTS_EXPECT( (!traits::is_specialized) &&
                        (!traits::is_exact) &&
                        (!traits::is_integer) &&
                        (traits::is_field) &&
                        (traits::is_commutative) &&
                        (traits::is_ordered),
                        "the primary template's flags");
        D_TESTS_EXPECT( (traits::zero().v == 0.0) &&
                        (traits::one().v == 1.0) &&
                        (d_tests_same_real(traits::epsilon().v,
                                           std::numeric_limits<double>::
                                               epsilon())) &&
                        (d_tests_same_real(traits::tolerance().v,
                                           4.0 *
                                           std::numeric_limits<double>::
                                               epsilon())) &&
                        (traits::magnitude(t_real(-3.5)).v == 3.5) &&
                        (traits::abs(t_real(-3.5)).v == 3.5) &&
                        (traits::to_double(t_real(0.75)) == 0.75) &&
                        (traits::from_double(-0.25).v == -0.25),
                        "the primary template, through double");
        D_TESTS_EXPECT( (d_tests_same_real(traits::sqrt(t_real(2.0)).v,
                                           std::sqrt(2.0))) &&
                        (d_tests_same_real(traits::sqrt(t_real(1e300)).v,
                                           std::sqrt(1e300))),
                        "the primary template's root, the double's");

        // an integral type with no traits of its own: exact, and an integer
        D_TESTS_EXPECT( (!dm::number_traits<char>::is_specialized) &&
                        (dm::number_traits<char>::is_exact) &&
                        (dm::number_traits<char>::is_integer) &&
                        (!dm::number_traits<char>::is_field) &&
                        (dm::number_traits<wchar_t>::is_integer) &&
                        (dm::number_traits<bool>::is_integer) &&
                        (dm::number_traits<char>::sqrt(99) == 9) &&
                        (dm::number_traits<char>::epsilon() == 0) &&
                        (dm::number_traits<char>::tolerance() == 0),
                        "the primary template's flags, an integral type, "
                        "its root exact");

        // the generic path over it, as it always was
        {
            const la::vector<t_real, 2> p(3.0, 4.0);
            const la::vector<t_real, 2> q(1.0, -2.0);

            D_TESTS_EXPECT( (p.norm().v == 5.0) &&
                            (p.plus(q).x().v == 4.0) &&
                            (p.minus(q).y().v == 6.0) &&
                            (p.scaled(t_real(2.0)).y().v == 8.0) &&
                            (p.divided(t_real(2.0)).x().v == 1.5) &&
                            (p.dot(q).v == -5.0) &&
                            (p == p) &&
                            (!(p == q)),
                            "the generic path over the primary template");
        }

        return;
    }


    //==========================================================================
    // 4.  NUMBER_TRAITS OF THE VALUE CLASSES
    //==========================================================================

    // d_tests_value_traits
    //   one class's traits.
    template<typename Value>
    void
    d_tests_value_traits(
        const char* _name
    )
    {
        typedef dm::number_traits<Value> traits;

        const Value two   = 2;
        const Value third = Value::ratio(1, 3);

        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (!traits::is_exact) &&
                        (!traits::is_integer) &&
                        (traits::is_field) &&
                        (traits::is_commutative) &&
                        (traits::is_ordered),
                        _name);

        // zero and one, exact; the tolerance four of a double's epsilons
        D_TESTS_EXPECT( (d_tests_is(traits::zero(), D_MATH_VALUE_INTEGER,
                                    Value(0))) &&
                        (d_tests_is(traits::one(), D_MATH_VALUE_INTEGER,
                                    Value(1))) &&
                        (d_tests_is(traits::tolerance(), D_MATH_VALUE_DOUBLE,
                                    Value(4.0 * DBL_EPSILON))),
                        _name);

        // the precision a value carries, by kind; 0 where it is exact
        D_TESTS_EXPECT( (d_tests_is(traits::epsilon(third),
                                    D_MATH_VALUE_INTEGER, Value(0))) &&
                        (d_tests_is(traits::epsilon(Value(0.5f)),
                                    D_MATH_VALUE_FLOAT, Value(FLT_EPSILON)))
                        &&
                        (d_tests_is(traits::epsilon(Value(0.5)),
                                    D_MATH_VALUE_DOUBLE, Value(DBL_EPSILON)))
                        &&
                        (d_tests_is(traits::epsilon(Value(0.5L)),
                                    D_MATH_VALUE_LONG_DOUBLE,
                                    Value(LDBL_EPSILON))),
                        _name);

        // a pair's tolerance: 0 where both are exact (S158), else four of
        // the larger epsilon times the larger magnitude (S157)
        D_TESTS_EXPECT( (d_tests_is(traits::tolerance(two, third),
                                    D_MATH_VALUE_INTEGER, Value(0))) &&
                        (traits::tolerance(two, Value(0.5)).compare(
                             Value(8.0 * DBL_EPSILON)) ==
                         D_MATH_VALUE_EQUAL) &&
                        (traits::tolerance(Value(0.5f), Value(-4.0)).compare(
                             Value(16.0 * FLT_EPSILON)) ==
                         D_MATH_VALUE_EQUAL) &&
                        (d_tests_is(traits::magnitude(Value(-3)),
                                    D_MATH_VALUE_INTEGER, Value(3))),
                        _name);

        // |x|: the class's own
        D_TESTS_EXPECT( (d_tests_is(traits::abs(Value(-3)),
                                    D_MATH_VALUE_INTEGER, Value(3))) &&
                        (d_tests_is(traits::abs(Value::ratio(-1, 2)),
                                    D_MATH_VALUE_RATIONAL,
                                    Value::ratio(1, 2))) &&
                        (d_tests_is(traits::abs(Value(-2.5)),
                                    D_MATH_VALUE_DOUBLE, Value(2.5))) &&
                        (traits::abs(Value(D_MATH_VALUE_ERROR_POLE)).kind() ==
                         D_MATH_VALUE_ERROR),
                        _name);

        // through double, and back; no number is a NaN
        D_TESTS_EXPECT( (d_tests_same_real(traits::to_double(third),
                                           1.0 / 3.0)) &&
                        (traits::to_double(two) == 2.0) &&
                        (traits::to_double(Value()) !=
                         traits::to_double(Value())) &&
                        (d_tests_is(traits::from_double(0.5),
                                    D_MATH_VALUE_DOUBLE, Value(0.5))),
                        _name);

        // the root: exact for the square of a rational of the family
        D_TESTS_EXPECT( (d_tests_is(traits::sqrt(Value(0)),
                                    D_MATH_VALUE_INTEGER, Value(0))) &&
                        (d_tests_is(traits::sqrt(Value(1)),
                                    D_MATH_VALUE_INTEGER, Value(1))) &&
                        (d_tests_is(traits::sqrt(Value(144)),
                                    D_MATH_VALUE_INTEGER, Value(12))) &&
                        (d_tests_is(traits::sqrt(Value::ratio(9, 49)),
                                    D_MATH_VALUE_RATIONAL,
                                    Value::ratio(3, 7))) &&
                        (d_tests_is(traits::sqrt(Value(2147395600L)),
                                    D_MATH_VALUE_INTEGER, Value(46340))),
                        _name);

        // otherwise the long double's, correctly rounded
        D_TESTS_EXPECT( (d_tests_is(traits::sqrt(two),
                                    D_MATH_VALUE_LONG_DOUBLE,
                                    Value(std::sqrt(2.0L)))) &&
                        (d_tests_is(traits::sqrt(third),
                                    D_MATH_VALUE_LONG_DOUBLE,
                                    Value(std::sqrt(1.0L / 3.0L)))) &&
                        (d_tests_is(traits::sqrt(Value(2.25)),
                                    D_MATH_VALUE_LONG_DOUBLE, Value(1.5L))),
                        _name);

        // an exact negative number has none; an inexact one's is a NaN; an
        // error is its own; what is no number, a domain error
        D_TESTS_EXPECT( (traits::sqrt(Value(-4)).kind() ==
                         D_MATH_VALUE_ERROR) &&
                        (traits::sqrt(Value::ratio(-1, 4)).kind() ==
                         D_MATH_VALUE_ERROR) &&
                        (traits::sqrt(Value(-2.0)).kind() ==
                         D_MATH_VALUE_LONG_DOUBLE) &&
                        (traits::sqrt(Value(-2.0)) !=
                         traits::sqrt(Value(-2.0))) &&
                        (traits::sqrt(Value(D_MATH_VALUE_ERROR_POLE))
                             .c_value().code ==
                         static_cast<int>(D_MATH_VALUE_ERROR_POLE)) &&
                        (traits::sqrt(Value()).c_value().code ==
                         static_cast<int>(D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (traits::sqrt(Value(true)).c_value().code ==
                         static_cast<int>(D_MATH_VALUE_ERROR_DOMAIN)),
                        _name);

        return;
    }

    // d_tests_wide_root
    //   a wide integer's root: past the family, through the long double; of
    // a negative one, none.
    void
    d_tests_wide_root()
    {
        dj_math_wide w = d_math_wide_from_imax(1);

        // 2^200
        for (int i = 0; i < 200; ++i)
        {
            (void)d_math_wide_add(&w, w, w);
        }

        {
            const dm::value big = dm::value::adopt(d_math_value_from_wide(w));
            const dm::value root = dm::number_traits<dm::value>::sqrt(big);
            const dm::value none = dm::number_traits<dm::value>::sqrt(-big);

            D_TESTS_EXPECT( (big.kind() == D_MATH_VALUE_BOXED) &&
                            (d_tests_is(root, D_MATH_VALUE_LONG_DOUBLE,
                                        dm::value(std::ldexp(1.0L, 100)))),
                            "a wide integer's root, its long double's");
            D_TESTS_EXPECT( (none.kind() == D_MATH_VALUE_ERROR) &&
                            (none.c_value().code ==
                             static_cast<int>(D_MATH_VALUE_ERROR_DOMAIN)),
                            "a negative wide integer's root, a domain error");
        }

        return;
    }


    //==========================================================================
    // 5.  COMMON NUMBER
    //==========================================================================

    // the arithmetic types, as a list
    struct d_tests_nil
    {};

    template<typename Head,
             typename Tail>
    struct d_tests_list
    {};

#if D_ENV_HAS_LONG_LONG
    typedef d_tests_list<t_llong,
            d_tests_list<t_ullong,
            d_tests_list<float,
            d_tests_list<double,
            d_tests_list<long double, d_tests_nil> > > > > t_wide_tail;
#else
    typedef d_tests_list<float,
            d_tests_list<double,
            d_tests_list<long double, d_tests_nil> > > t_wide_tail;
#endif

    typedef d_tests_list<bool,
            d_tests_list<char,
            d_tests_list<signed char,
            d_tests_list<unsigned char,
            d_tests_list<short,
            d_tests_list<unsigned short,
            d_tests_list<int,
            d_tests_list<unsigned,
            d_tests_list<long,
            d_tests_list<unsigned long, t_wide_tail> > > > > > > > > >
        t_arithmetic;

    long g_pairs = 0;

    // d_tests_common_pair
    //   common_number<A, B> against the type a + b has.
    template<typename A,
             typename B>
    void
    d_tests_common_pair()
    {
        typedef typename dm::common_number<A, B>::type common;

        const A a = static_cast<A>(1);
        const B b = static_cast<B>(1);

        ++g_pairs;
        D_TESTS_EXPECT(d_tests_type_of(a + b) == d_tests_type_of(common()),
                       "common_number, an arithmetic pair");

        return;
    }

    // d_tests_pairs_with
    //   A with every type in List.
    template<typename A,
             typename List>
    struct d_tests_pairs_with;

    template<typename A>
    struct d_tests_pairs_with<A, d_tests_nil>
    {
        static void
        run()
        {}
    };

    template<typename A,
             typename Head,
             typename Tail>
    struct d_tests_pairs_with<A, d_tests_list<Head, Tail> >
    {
        static void
        run()
        {
            d_tests_common_pair<A, Head>();
            d_tests_pairs_with<A, Tail>::run();
        }
    };

    // d_tests_all_pairs
    //   every type in List with every type in All.
    template<typename List,
             typename All>
    struct d_tests_all_pairs;

    template<typename All>
    struct d_tests_all_pairs<d_tests_nil, All>
    {
        static void
        run()
        {}
    };

    template<typename Head,
             typename Tail,
             typename All>
    struct d_tests_all_pairs<d_tests_list<Head, Tail>, All>
    {
        static void
        run()
        {
            d_tests_pairs_with<Head, All>::run();
            d_tests_all_pairs<Tail, All>::run();
        }
    };

    // d_tests_value_pair
    //   common_number for a value class with an arithmetic type, both ways,
    // against the operators' types.
    template<typename Value,
             typename A>
    void
    d_tests_value_pair()
    {
        typedef typename dm::common_number<Value, A>::type left;
        typedef typename dm::common_number<A, Value>::type right;

        const Value v = 1;
        const A     a = static_cast<A>(1);

        D_TESTS_EXPECT( (d_tests_type_of(v + a) == d_tests_type_of(left())) &&
                        (d_tests_type_of(a * v) == d_tests_type_of(right())),
                        "common_number, a value class and a number");

        return;
    }

    // d_tests_common_number
    //   every pair.
    void
    d_tests_common_number()
    {
        const dm::inline_value iv = 1;
        const dm::value        v  = 1;

        d_tests_all_pairs<t_arithmetic, t_arithmetic>::run();

        d_tests_value_pair<dm::inline_value, bool>();
        d_tests_value_pair<dm::inline_value, char>();
        d_tests_value_pair<dm::inline_value, int>();
        d_tests_value_pair<dm::inline_value, unsigned long>();
        d_tests_value_pair<dm::inline_value, double>();
        d_tests_value_pair<dm::inline_value, long double>();
        d_tests_value_pair<dm::value, short>();
        d_tests_value_pair<dm::value, int>();
        d_tests_value_pair<dm::value, unsigned>();
        d_tests_value_pair<dm::value, float>();
        d_tests_value_pair<dm::value, double>();

        D_TESTS_EXPECT( (d_tests_type_of(iv + iv) ==
                         d_tests_type_of(dm::common_number<
                             dm::inline_value, dm::inline_value>::type())) &&
                        (d_tests_type_of(iv + v) ==
                         d_tests_type_of(dm::common_number<
                             dm::inline_value, dm::value>::type())) &&
                        (d_tests_type_of(v - iv) ==
                         d_tests_type_of(dm::common_number<
                             dm::value, dm::inline_value>::type())) &&
                        (d_tests_type_of(v * v) ==
                         d_tests_type_of(dm::common_number<
                             dm::value, dm::value>::type())),
                        "common_number, the value classes together");

        return;
    }


    //==========================================================================
    // 6.  LINEAR ALGEBRA OVER A VALUE
    //==========================================================================

    // d_tests_hilbert
    //   the n x n Hilbert matrix, 1 / (i + j + 1), in Type, by Make.
    template<typename Type,
             std::size_t N>
    la::matrix<Type, N, N>
    d_tests_hilbert(
        Type (*_ratio)(long, long)
    )
    {
        Type entries[N * N];

        for (std::size_t i = 0; i < N; ++i)
        {
            for (std::size_t j = 0; j < N; ++j)
            {
                entries[(i * N) + j] =
                    _ratio(1L, static_cast<long>(i + j + 1));
            }
        }

        return la::matrix<Type, N, N>(entries);
    }

    dm::value
    d_tests_value_ratio(
        long _n,
        long _d
    )
    {
        return dm::value::ratio(_n, _d);
    }

    t_ratio
    d_tests_t_ratio(
        long _n,
        long _d
    )
    {
        return t_ratio(_n, _d);
    }

    // d_tests_hilbert_exact
    //   the Hilbert matrix of order 3, and of 4 where the family's word is
    // 64 bits: its determinant, 1 / 2160 and 1 / 6048000, and its inverse,
    // whose entries are integers, exactly; and the product the identity.
    template<typename Type>
    void
    d_tests_hilbert_exact(
        Type (*_ratio)(long, long),
        bool        _four,
        const char* _name
    )
    {
        typedef dm::number_traits<Type> traits;

        const la::matrix<Type, 3, 3> h3 = d_tests_hilbert<Type, 3>(_ratio);
        const la::matrix<Type, 3, 3> i3 = la::inverse(h3);
        const long                   want3[9] = { 9,   -36,  30,
                                                  -36, 192,  -180,
                                                  30,  -180, 180 };
        bool                         entries_ok = true;

        for (std::size_t k = 0; k < 9; ++k)
        {
            entries_ok = ( (entries_ok) &&
                           (i3(k / 3, k % 3) ==
                            _ratio(want3[k], 1L)) );
        }

        D_TESTS_EXPECT( (la::determinant(h3) == _ratio(1L, 2160L)) &&
                        (entries_ok) &&
                        (h3.times(i3) ==
                         la::matrix<Type, 3, 3>::identity()) &&
                        (i3.times(h3).is_identity(traits::zero())) &&
                        (h3.is_symmetric(traits::zero())) &&
                        (la::is_invertible(h3, traits::zero())),
                        _name);

        // the solution, by elimination of [H | B] (S257): exactly the
        // inverse's times B, and H times it B; a vector's the same column
        {
            la::matrix<Type, 3, 2> b;

            b(0, 0) = _ratio(1L, 1L);
            b(1, 0) = _ratio(-2L, 1L);
            b(2, 0) = _ratio(1L, 2L);
            b(0, 1) = _ratio(0L, 1L);
            b(1, 1) = _ratio(3L, 7L);
            b(2, 1) = _ratio(-1L, 1L);

            const la::matrix<Type, 3, 2> x = la::solve(h3, b);
            const la::vector<Type, 3>    v = la::solve(h3, b.col(1));

            D_TESTS_EXPECT( (x == i3.times(b)) &&
                            (h3.times(x) == b) &&
                            (v == x.col(1)) &&
                            (x(0, 0) == _ratio(96L, 1L)),
                            _name);
        }

        // a fourth order where the terms hold it
        if (_four)
        {
            const la::matrix<Type, 4, 4> h4 = d_tests_hilbert<Type, 4>(_ratio);
            const la::matrix<Type, 4, 4> i4 = la::inverse(h4);

            D_TESTS_EXPECT( (la::determinant(h4) == _ratio(1L, 6048000L)) &&
                            (i4(0, 0) == _ratio(16L, 1L)) &&
                            (i4(2, 2) == _ratio(6480L, 1L)) &&
                            (i4(3, 1) == _ratio(1680L, 1L)) &&
                            (h4.times(i4) ==
                             la::matrix<Type, 4, 4>::identity()),
                            _name);
        }

        return;
    }

    // d_tests_value_vectors
    //   vectors of values: zero by default, exact arithmetic, exact norms
    // where the norm is a rational.
    void
    d_tests_value_vectors()
    {
        typedef la::vector<dm::value, 3> v3;
        typedef la::vector<dm::value, 2> v2;

        const v3        zero;
        const v3        a(1, 2, 2);
        const v3        b(dm::value::ratio(1, 2), dm::value(-1),
                          dm::value::ratio(1, 3));
        const v2        p(3, 4);
        const v2        unit = p.normalized();
        const dm::value half = dm::value::ratio(1, 2);

        D_TESTS_EXPECT( (d_tests_is(zero.x(), D_MATH_VALUE_INTEGER,
                                    dm::value(0))) &&
                        (d_tests_is(v3::zeros().z(), D_MATH_VALUE_INTEGER,
                                    dm::value(0))) &&
                        (d_tests_is(v3::basis<1>().y(), D_MATH_VALUE_INTEGER,
                                    dm::value(1))) &&
                        (d_tests_is(v3::basis<1>().x(), D_MATH_VALUE_INTEGER,
                                    dm::value(0))),
                        "a vector of values: zero by default");
        D_TESTS_EXPECT( (d_tests_is(a.norm(), D_MATH_VALUE_INTEGER,
                                    dm::value(3))) &&
                        (d_tests_is(p.norm(), D_MATH_VALUE_INTEGER,
                                    dm::value(5))) &&
                        (d_tests_is(unit.x(), D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(3, 5))) &&
                        (d_tests_is(unit.y(), D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(4, 5))) &&
                        (unit.is_unit()) &&
                        (d_tests_is(p.distance(v2(0, 0)), D_MATH_VALUE_INTEGER,
                                    dm::value(5))),
                        "a vector of values: exact norms");
        D_TESTS_EXPECT( (d_tests_is(a.dot(b), D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(-5, 6))) &&
                        (a.cross(b) == v3(dm::value::ratio(8, 3),
                                          dm::value::ratio(2, 3),
                                          dm::value::ratio(-2, 1))) &&
                        (a.plus(b).minus(b) == a) &&
                        (a.scaled(half).divided(half) == a) &&
                        (d_tests_is(a.sum(), D_MATH_VALUE_INTEGER,
                                    dm::value(5))) &&
                        (d_tests_is(b.min_coeff(), D_MATH_VALUE_INTEGER,
                                    dm::value(-1))) &&
                        (d_tests_is(b.projected_onto(a).x(),
                                    D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(-5, 54))),
                        "a vector of values: exact arithmetic");

        // an angle, through double: a double value, compared once stored,
        // as the x87 hands back the C library's arc-cosine with bits a
        // double has not, which an optimizing build may keep
        {
            const dm::value right = v2(1, 0).angle_to(v2(0, 1));
            double          angle = 0.0;

            D_TESTS_EXPECT( (right.kind() == D_MATH_VALUE_DOUBLE) &&
                            (right.to(angle)) &&
                            (d_tests_same_real(angle, 1.5707963267948966)) &&
                            (d_tests_is(v2(2, 0).angle_to(v2(5, 0)),
                                        D_MATH_VALUE_DOUBLE,
                                        dm::value(0.0))),
                            "a vector of values: an angle, through double");
        }

        return;
    }

    // d_tests_value_matrices
    //   matrices of values, and the Hilbert matrix.
    void
    d_tests_value_matrices()
    {
        typedef la::matrix<dm::value, 2, 2> m2;

        const dm::value entries[4] = { dm::value(2), dm::value::ratio(1, 2),
                                       dm::value(-1), dm::value(3) };
        const m2        a(entries);
        const m2        inv = la::inverse(a);

        D_TESTS_EXPECT( (d_tests_is(m2()(1, 0), D_MATH_VALUE_INTEGER,
                                    dm::value(0))) &&
                        (d_tests_is(m2::identity()(1, 1), D_MATH_VALUE_INTEGER,
                                    dm::value(1))) &&
                        (d_tests_is(la::determinant(a), D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(13, 2))) &&
                        (a.times(inv) == m2::identity()) &&
                        (d_tests_is(inv(0, 1), D_MATH_VALUE_RATIONAL,
                                    dm::value::ratio(-1, 13))) &&
                        (d_tests_is(a.trace(), D_MATH_VALUE_INTEGER,
                                    dm::value(5))),
                        "a matrix of values: exact");

        d_tests_hilbert_exact<dm::value>(d_tests_value_ratio,
                                         (sizeof(dj_math_imax) >= 8u),
                                         "the Hilbert matrix of values");

        // inexact values on the generic path: each pivot the entry of
        // largest magnitude -- with none, [1e-20 1; 1 1] x = (1, 2) loses
        // x's first entry, 0 for 1, and the inverse its corner; with it,
        // both are right to rounding
        {
            typedef dm::number_traits<dm::value> traits;

            m2                       s;
            la::vector<dm::value, 2> rhs;

            s(0, 0) = dm::value(1e-20);
            s(0, 1) = dm::value(1);
            s(1, 0) = dm::value(1);
            s(1, 1) = dm::value(1);
            rhs[0]  = dm::value(1);
            rhs[1]  = dm::value(2);

            {
                const la::vector<dm::value, 2> x   = la::solve(s, rhs);
                const m2                       inv2 = la::inverse(s);

                D_TESTS_EXPECT( (d_tests_same_real(traits::to_double(x[0]),
                                                   1.0)) &&
                                (d_tests_same_real(traits::to_double(x[1]),
                                                   1.0)) &&
                                (d_tests_same_real(
                                    traits::to_double(inv2(0, 0)), -1.0)) &&
                                (d_tests_same_real(
                                    traits::to_double(inv2(1, 0)), 1.0)),
                                "a matrix of inexact values: solve and "
                                "inverse pivot by magnitude");
            }
        }

        return;
    }

    // d_tests_wide_vectors
    //   wide integers in a vector: the dot product past dj_math_imax, exact,
    // and every box given back once the vectors are.
    void
    d_tests_wide_vectors()
    {
        const long live = g_live;

        {
            dj_math_wide w = d_math_wide_from_imax(1);

            // 2^100
            for (int i = 0; i < 100; ++i)
            {
                (void)d_math_wide_add(&w, w, w);
            }

            const dm::value big = dm::value::adopt(d_math_value_from_wide(w));
            const la::vector<dm::value, 3> a(big, dm::value(1), dm::value(2));
            const la::vector<dm::value, 3> b(big, dm::value(3), dm::value(4));
            const dm::value                square = big * big;
            const dm::value                dot    = a.dot(b);

            D_TESTS_EXPECT( (square.kind() == D_MATH_VALUE_BOXED) &&
                            (dot.kind() == D_MATH_VALUE_BOXED) &&
                            (dot == square + 11) &&
                            (dot - square == 11) &&
                            (a.cross(b).x() == 4 - 6) &&
                            (a.cross(b).y() == (2 * big) - (4 * big)),
                            "wide integers in a vector: exact past "
                            "dj_math_imax");
        }

        D_TESTS_EXPECT( (g_live == live),
                        "wide integers in a vector: every box given back");

        return;
    }


    //==========================================================================
    // 7.  LINEAR ALGEBRA OVER THE TEST'S OWN RATIONAL
    //==========================================================================

    // d_tests_ratio_algebra
    //   vectors and matrices of t_ratio, which only its traits make known.
    void
    d_tests_ratio_algebra()
    {
        typedef la::vector<t_ratio, 3>      v3;
        typedef la::vector<t_ratio, 2>      v2;
        typedef dm::number_traits<t_ratio> traits;

        const v3 a(1, 2, 2);
        const v3 b(t_ratio(1L, 2L), t_ratio(-1), t_ratio(1L, 3L));
        const v2 p(3, 4);

        D_TESTS_EXPECT( (v3() == v3(0, 0, 0)) &&
                        (v3::basis<2>() == v3(0, 0, 1)) &&
                        (a.norm() == t_ratio(3)) &&
                        (p.normalized() == v2(t_ratio(3L, 5L),
                                              t_ratio(4L, 5L))) &&
                        (a.dot(b) == t_ratio(-5L, 6L)) &&
                        (a.cross(b) == v3(t_ratio(8L, 3L), t_ratio(2L, 3L),
                                          t_ratio(-2))) &&
                        (a.equals(a, t_ratio(0))) &&
                        (!a.equals(b)),
                        "a vector of the test's rationals: exact");

        // a comparison given no tolerance takes the traits': a thousandth
        D_TESTS_EXPECT( (a.equals(v3(t_ratio(2001L, 2000L), t_ratio(2),
                                     t_ratio(2)))) &&
                        (!a.equals(v3(t_ratio(501L, 500L), t_ratio(2),
                                      t_ratio(2)))),
                        "the test's rationals' tolerance, by default");
        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (traits::is_exact) &&
                        (!traits::is_integer) &&
                        (traits::is_field) &&
                        (traits::is_commutative) &&
                        (traits::is_ordered) &&
                        (traits::magnitude(t_ratio(-3L, 4L)) ==
                         t_ratio(3L, 4L)) &&
                        (traits::epsilon() == t_ratio(0)) &&
                        (traits::tolerance(a.x(), b.x()) ==
                         t_ratio(1L, 1000L)) &&
                        (traits::sqrt(t_ratio(9L, 4L)) == t_ratio(3L, 2L)) &&
                        (traits::sqrt(t_ratio(2)) == t_ratio(92682L, 65536L)),
                        "the test's rationals' traits");

        d_tests_hilbert_exact<t_ratio>(d_tests_t_ratio, false,
                                       "the Hilbert matrix of the test's "
                                       "rationals");

        return;
    }

    // d_tests_q
    //   the quaternion w + x i + y j + z k of integers.
    t_quat
    d_tests_q(
        int _w,
        int _x,
        int _y,
        int _z
    )
    {
        return t_quat(t_ratio(_w), t_ratio(_x), t_ratio(_y), t_ratio(_z));
    }

    // d_tests_quaternion_algebra
    //   matrices of t_quat, whose products do not commute: the generic
    // inverse and solution keep each product in its order (S269) -- with
    // the order wrong, A A^-1, A^-1 A and A X all miss -- the first pivot
    // a row swap, by the squared norm.
    void
    d_tests_quaternion_algebra()
    {
        typedef la::matrix<t_quat, 3, 3> q33;
        typedef la::matrix<t_quat, 3, 2> q32;
        typedef dm::number_traits<t_quat> traits;

        const t_quat i = d_tests_q(0, 1, 0, 0);
        const t_quat j = d_tests_q(0, 0, 1, 0);
        const t_quat k = d_tests_q(0, 0, 0, 1);
        q33          a;
        q32          b;

        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (traits::is_exact) &&
                        (traits::is_field) &&
                        (!traits::is_commutative) &&
                        (!traits::is_ordered) &&
                        ((i * j) == k) &&
                        ((j * i) == -k) &&
                        (traits::magnitude(d_tests_q(1, 1, 1, 1)) ==
                         t_ratio(4)),
                        "the test's quaternions: products not commuting");

        a(0, 0) = d_tests_q(1, 0, 1, 0);
        a(0, 1) = d_tests_q(0, 1, 0, 0);
        a(0, 2) = d_tests_q(2, 0, 0, 0);
        a(1, 0) = d_tests_q(0, 1, 1, 1);
        a(1, 1) = d_tests_q(1, 0, 0, 1);
        a(1, 2) = d_tests_q(0, 0, 1, 0);
        a(2, 0) = d_tests_q(0, 0, 0, 1);
        a(2, 1) = d_tests_q(1, 1, 0, 0);
        a(2, 2) = d_tests_q(0, 1, 0, 2);

        b(0, 0) = d_tests_q(1, 0, 0, 0);
        b(0, 1) = d_tests_q(0, 0, 1, 0);
        b(1, 0) = d_tests_q(0, 1, 0, 0);
        b(1, 1) = d_tests_q(2, 0, 0, 1);
        b(2, 0) = d_tests_q(0, 0, 0, 1);
        b(2, 1) = d_tests_q(1, 1, 1, 1);

        {
            const q33                   inv = la::inverse(a);
            const q32                   x   = la::solve(a, b);
            const la::vector<t_quat, 3> v   = la::solve(a, b.col(0));

            D_TESTS_EXPECT( (a.times(inv) == q33::identity()) &&
                            (inv.times(a) == q33::identity()) &&
                            (inv(0, 0) == t_quat(t_ratio(1L, 3L),
                                                 t_ratio(1L, 6L),
                                                 t_ratio(1L, 6L),
                                                 t_ratio(0))),
                            "the test's quaternions: the inverse, exact, "
                            "on both sides");
            D_TESTS_EXPECT( (a.times(x) == b) &&
                            (x == inv.times(b)) &&
                            (v == x.col(0)) &&
                            (x(0, 0) == t_quat(t_ratio(7L, 18L),
                                               t_ratio(-5L, 18L),
                                               t_ratio(-2L, 3L),
                                               t_ratio(2L, 9L))),
                            "the test's quaternions: the solution, exact");
        }

        return;
    }


    //==========================================================================
    // 8.  CONSTANT EVALUATION
    //==========================================================================

#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )

    // d_tests_ce_dot
    //   (1, 2, 3) . (4, 5, 6), of inline values.
    constexpr dm::inline_value
    d_tests_ce_dot()
    {
        return la::vector<dm::inline_value, 3>(1, 2, 3)
            .dot(la::vector<dm::inline_value, 3>(4, 5, 6));
    }

    // d_tests_ce_norm
    //   the norm of (3, 4), exactly 5.
    constexpr dm::inline_value
    d_tests_ce_norm()
    {
        return la::vector<dm::inline_value, 2>(3, 4).norm();
    }

    // d_tests_ce_zero
    //   a vector of inline values made by default: the integer 0.
    constexpr bool
    d_tests_ce_zero()
    {
        return ( (la::vector<dm::inline_value, 3>().y() == 0) &&
                 (la::vector<dm::inline_value, 3>().y().kind() ==
                  D_MATH_VALUE_INTEGER) );
    }

    // d_tests_ce_hilbert
    //   the Hilbert matrix of order 3 of inline values: 1 / its
    // determinant, and its inverse's middle entry.
    constexpr dm::inline_value
    d_tests_ce_hilbert(
        bool _inverse
    )
    {
        const dm::inline_value entries[9] = {
            dm::inline_value::ratio(1, 1), dm::inline_value::ratio(1, 2),
            dm::inline_value::ratio(1, 3), dm::inline_value::ratio(1, 2),
            dm::inline_value::ratio(1, 3), dm::inline_value::ratio(1, 4),
            dm::inline_value::ratio(1, 3), dm::inline_value::ratio(1, 4),
            dm::inline_value::ratio(1, 5) };
        const la::matrix<dm::inline_value, 3, 3> h(entries);

        return (_inverse) ? la::inverse(h)(1, 1)
                          : (dm::inline_value(1) / la::determinant(h));
    }

#endif  // C++14

}  // namespace

#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
static_assert(d_tests_ce_dot() == 32,
              "a dot product of inline values, a constant expression");
static_assert( (d_tests_ce_norm() == 5) &&
               (d_tests_ce_norm().kind() == D_MATH_VALUE_INTEGER),
               "an exact norm, a constant expression");
static_assert(d_tests_ce_zero(), "a zero vector, a constant expression");
static_assert( (d_tests_ce_hilbert(false) == 2160) &&
               (d_tests_ce_hilbert(true) == 192),
               "the Hilbert matrix's determinant and inverse, constant "
               "expressions");
static_assert( (dm::number_traits<double>::sqrt(2.25) == 1.5) &&
               (dm::number_traits<int>::sqrt(99) == 9) &&
               (dm::number_traits<dm::inline_value>::sqrt(
                    dm::inline_value::ratio(25, 4)) ==
                dm::inline_value::ratio(5, 2)),
               "number_traits' roots, constant expressions");
#endif

#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
namespace
{
    // d_tests_ce_value_norm
    //   the norm of (5, 12) of values, exactly 13.
    constexpr dm::value
    d_tests_ce_value_norm()
    {
        return la::vector<dm::value, 2>(5, 12).norm();
    }
}  // namespace

static_assert( (d_tests_ce_value_norm() == 13) &&
               (d_tests_ce_value_norm().kind() == D_MATH_VALUE_INTEGER),
               "an exact norm of values, a constant expression (C++20)");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert( (dm::number<int>) &&
               (dm::number<long double>) &&
               (dm::number<dm::inline_value>) &&
               (dm::number<dm::value>) &&
               (dm::number<t_ratio>) &&
               (dm::number<t_real>) &&
               (!dm::number<bool>) &&
               (!dm::number<const char*>),
               "the concept number");
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
static_assert( (d_tests_same_type<dm::common_number_t<char, short>,
                                  int>::value) &&
               (d_tests_same_type<dm::common_number_t<unsigned, long>,
                                  decltype(1u + 1L)>::value) &&
               (d_tests_same_type<dm::common_number_t<float, int>,
                                  float>::value) &&
               (d_tests_same_type<dm::common_number_t<dm::inline_value,
                                                      double>,
                                  dm::inline_value>::value) &&
               (d_tests_same_type<dm::common_number_t<dm::inline_value,
                                                      dm::value>,
                                  dm::value>::value) &&
               (d_tests_same_type<dm::common_number_t<t_ratio, t_ratio>,
                                  t_ratio>::value),
               "common_number_t (C++11)");
#endif

namespace
{
    // d_tests_constant_evaluation
    //   the constant expressions, at run time.
    void
    d_tests_constant_evaluation()
    {
#if ( D_ENV_LANG_IS_CPP14_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
        D_TESTS_EXPECT( (d_tests_ce_dot() == 32) &&
                        (d_tests_ce_norm() == 5) &&
                        (d_tests_ce_zero()) &&
                        (d_tests_ce_hilbert(false) == 2160) &&
                        (d_tests_ce_hilbert(true) == 192),
                        "the constant expressions, at run time");
#endif
#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
        D_TESTS_EXPECT(d_tests_ce_value_norm() == 13,
                       "a value's constant expression, at run time");
#endif

        return;
    }
}  // namespace


int
main()
{
    d_tests_arithmetic_traits();
    d_tests_primary_traits();
    d_tests_value_traits<dm::inline_value>("inline_value's traits");
    d_tests_value_traits<dm::value>("value's traits");
    d_tests_wide_root();
    d_tests_common_number();
    d_tests_value_vectors();
    d_tests_value_matrices();
    d_tests_wide_vectors();
    d_tests_ratio_algebra();
    d_tests_quaternion_algebra();
    d_tests_constant_evaluation();

    // every box given back
    D_TESTS_EXPECT( (g_live == 0) &&
                    (g_made > 0) &&
                    (g_pairs > 0),
                    "every box given back");

    std::printf("t_number_traits_face: %ld checks, %ld failures\n", g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
