/*******************************************************************************
* djinterp [cpp][math]                                              sequence.hpp
*
* Sequences.
*   A sequence maps an index n (0-based) to a term value. A general sequence
* wraps any index->value functor; named sequences (arithmetic, geometric,
* power, Fibonacci) provide closed-form terms and partial sums.
*
*   sequence<Gen>                 - wrap a functor n -> value
*   make_sequence(gen)            - deduce and build one
*   arithmetic_sequence<T>(a0,d)  - a0 + n d
*   geometric_sequence<T>(a0,r)   - a0 r^n
*   power_sequence<T>(p)          - n^p (integer p)
*   fibonacci_sequence            - 0,1,1,2,3,5,...
*
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 the arithmetic sequence's term and partial sum, and each
* sequence's construction and term of a constexpr functor, are constant
* expressions. A term or partial sum that loops as many times as its index
* or count says (the general sequence's partial sum, the geometric, power
* and Fibonacci terms, the geometric partial sum) is a constant expression
* from C++14, where a constexpr function may loop; at C++11 it is the same
* loop at run time, so its bits are the same at every level.
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/sequence.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_SEQUENCE_HPP
#define DJINTERP_MATH_CALCULUS_SEQUENCE_HPP 1

// djinterp
#include "../../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    GENERAL SEQUENCE
// ============================================================================

// sequence
//   wraps an index->value functor. Constexpr term access when the functor is
// constexpr (use a functor object; lambdas are not constexpr before C++17).
template<typename _Gen, typename _Value = double>
struct sequence
{
    using value_type = _Value;

    _Gen gen;

    D_CONSTEXPR explicit sequence(_Gen _g) : gen(_g) {}

    D_CONSTEXPR value_type term(std::size_t _n) const
    { return static_cast<value_type>(gen(_n)); }

    D_CONSTEXPR value_type operator[](std::size_t _n) const
    { return term(_n); }

    // partial_sum: sum of the first _count terms (indices 0 .. _count-1).
    D_CONSTEXPR_CPP14 value_type partial_sum(std::size_t _count) const
    {
        value_type acc = value_type(0);
        for (std::size_t n = 0; n < _count; ++n) { acc += term(n); }
        return acc;
    }
};

template<typename _Gen>
D_CONSTEXPR
sequence<_Gen> make_sequence(
    _Gen _g
)
{
    return sequence<_Gen>(_g);
}


// ============================================================================
// II.   ARITHMETIC SEQUENCE  ( a0 + n d )
// ============================================================================

template<typename _T = double>
struct arithmetic_sequence
{
    using value_type = _T;

    _T a0;
    _T d;

    D_CONSTEXPR arithmetic_sequence(_T _a0, _T _d) : a0(_a0), d(_d) {}

    D_CONSTEXPR _T term(std::size_t _n) const
    { return a0 + static_cast<_T>(_n) * d; }

    D_CONSTEXPR _T operator[](std::size_t _n) const
    { return term(_n); }

    // partial_sum: first _count terms, n(2 a0 + (n-1) d) / 2.
    D_CONSTEXPR _T partial_sum(std::size_t _count) const
    {
        return static_cast<_T>(_count) * a0
             + d * static_cast<_T>(_count * (_count - 1) / 2);
    }
};


// ============================================================================
// III.  GEOMETRIC SEQUENCE  ( a0 r^n )
// ============================================================================

template<typename _T = double>
struct geometric_sequence
{
    using value_type = _T;

    _T a0;
    _T r;

    D_CONSTEXPR geometric_sequence(_T _a0, _T _r) : a0(_a0), r(_r) {}

    D_CONSTEXPR_CPP14 _T term(std::size_t _n) const
    {
        _T p = _T(1);
        for (std::size_t i = 0; i < _n; ++i) { p *= r; }
        return a0 * p;
    }

    D_CONSTEXPR_CPP14 _T operator[](std::size_t _n) const
    { return term(_n); }

    // partial_sum: a0 (1 - r^count) / (1 - r), or count*a0 when r == 1.
    D_CONSTEXPR_CPP14 _T partial_sum(std::size_t _count) const
    {
        if (r == _T(1)) { return static_cast<_T>(_count) * a0; }

        _T rn = _T(1);
        for (std::size_t i = 0; i < _count; ++i) { rn *= r; }
        return a0 * (_T(1) - rn) / (_T(1) - r);
    }
};


// ============================================================================
// IV.   POWER SEQUENCE  ( n^p )  and  FIBONACCI
// ============================================================================

template<typename _T = double>
struct power_sequence
{
    using value_type = _T;

    unsigned p;

    D_CONSTEXPR explicit power_sequence(unsigned _p) : p(_p) {}

    D_CONSTEXPR_CPP14 _T term(std::size_t _n) const
    {
        _T acc = _T(1);
        _T base = static_cast<_T>(_n);
        for (unsigned i = 0; i < p; ++i) { acc *= base; }
        return acc;
    }

    D_CONSTEXPR_CPP14 _T operator[](std::size_t _n) const
    { return term(_n); }
};

struct fibonacci_sequence
{
    using value_type = unsigned long long;

    D_CONSTEXPR_CPP14
    value_type term(
        std::size_t _n
    ) const
    {
        value_type a = 0;
        value_type b = 1;
        for (std::size_t i = 0; i < _n; ++i)
        {
            value_type next = a + b;
            a = b;
            b = next;
        }
        return a;
    }

    D_CONSTEXPR_CPP14 value_type operator[](std::size_t _n) const
    { return term(_n); }
};

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_CALCULUS_SEQUENCE_HPP
