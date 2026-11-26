/*******************************************************************************
* djinterp [cpp][math]                                           t_lazy_face.cpp
*
*   Tests for math/linear_algebra/lazy.hpp, the opt-in lazy type (step 34,
* 5f), at every level from C++98. Below C++11 the header is empty. From
* C++11: lazy expressions of vectors, matrices and dynamic matrices, of
* double, float, long double and math::complex<double>, evaluated by eval,
* e.eval() and assign, each element the eager arithmetic's bit for bit, at
* 300 random draws: sums, differences, negations, scalings, a vector divided,
* entry-wise products, matrix products by a matrix and by a vector, and
* transposes; assign in place, the destination read in the expression, and
* into a dynamic matrix of another shape; the types, shapes and members.
* Where clang evaluates double wider than double, a 32-bit x87, its eager
* fixed-size arithmetic keeps the wider value across its steps (S37), and
* the fixed-size checks are left out; the dynamic matrix's, whose eager
* steps are each stored, stand.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/linear_algebra/t_lazy_face.cpp       \
*         -o t_lazy -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_lazy_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/lazy.hpp"  // tested
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::memcmp


namespace
{
    long g_checks   = 0;
    long g_failures = 0;

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

}  // namespace


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>  // std::is_same
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/math/complex.hpp"  // complex
#include "../../../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::
                                                                 // uint64_t

// D_TESTS_FIXED_BITS
//   1 where eager fixed-size arithmetic stores each step, so that the lazy
// type gives its bits: everywhere but clang on a target that evaluates
// double wider than double (S37).
#if ( defined(__clang__) && D_LINALG_LAZY_WIDE )
    #define D_TESTS_FIXED_BITS 0
#else
    #define D_TESTS_FIXED_BITS 1
#endif

namespace
{
    namespace dm = djinterp::math;
    namespace la = djinterp::math::linalg;

    typedef dm::complex<double> cd;

    re_std::uint64_t g_seed = 88172645463325252ull;

    // d_tests_draw
    //   a number in [-1, 1), by xorshift.
    double
    d_tests_draw()
    {
        g_seed ^= g_seed << 13;
        g_seed ^= g_seed >> 7;
        g_seed ^= g_seed << 17;

        return ( static_cast<double>(g_seed >> 11) *
                 (1.0 / 9007199254740992.0) * 2.0 ) - 1.0;
    }

    // d_tests_same
    //   whether two values have the same bytes, in a value's own size.
    template<typename Type>
    bool
    d_tests_same(
        const Type& _a,
        const Type& _b
    )
    {
        return (std::memcmp(&_a, &_b, sizeof(Type)) == 0);
    }

    // a long double's padding bytes are none of its value
    bool
    d_tests_same(
        const long double& _a,
        const long double& _b
    )
    {
        return ( (_a == _b) || ((_a != _a) && (_b != _b)) );
    }

    // d_tests_same_vectors, _matrices, _dynamic
    //   whether two values hold the same bytes, element by element.
    template<typename Type, std::size_t N>
    bool
    d_tests_same_vectors(
        const la::vector<Type, N>& _a,
        const la::vector<Type, N>& _b
    )
    {
        bool same = true;

        for (std::size_t i = 0; i < N; ++i)
        {
            same = same && d_tests_same(_a[i], _b[i]);
        }

        return same;
    }

    template<typename Type, std::size_t Rows, std::size_t Cols>
    bool
    d_tests_same_matrices(
        const la::matrix<Type, Rows, Cols>& _a,
        const la::matrix<Type, Rows, Cols>& _b
    )
    {
        bool same = true;

        for (std::size_t i = 0; i < (Rows * Cols); ++i)
        {
            same = same && d_tests_same(_a.data()[i], _b.data()[i]);
        }

        return same;
    }

    template<typename Type>
    bool
    d_tests_same_dynamic(
        const la::dynamic_matrix<Type>& _a,
        const la::dynamic_matrix<Type>& _b
    )
    {
        bool same = ( (_a.rows() == _b.rows()) && (_a.cols() == _b.cols()) );

        for (std::size_t i = 0; same && (i < _a.data().size()); ++i)
        {
            same = d_tests_same(_a.data()[i], _b.data()[i]);
        }

        return same;
    }

    // d_tests_fill
    //   a vector, a matrix or a dynamic matrix of draws, scaled.
    template<typename Type, std::size_t N>
    void
    d_tests_fill(
        la::vector<Type, N>& _v,
        double               _scale
    )
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            _v[i] = static_cast<Type>(d_tests_draw() * _scale);
        }
    }

    template<std::size_t N>
    void
    d_tests_fill(
        la::vector<cd, N>& _v,
        double             _scale
    )
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            const double re = d_tests_draw() * _scale;

            _v[i] = cd(re, d_tests_draw() * _scale);
        }
    }

    template<typename Type, std::size_t Rows, std::size_t Cols>
    void
    d_tests_fill(
        la::matrix<Type, Rows, Cols>& _m,
        double                        _scale
    )
    {
        for (std::size_t i = 0; i < Rows; ++i)
        {
            for (std::size_t j = 0; j < Cols; ++j)
            {
                _m(i, j) = static_cast<Type>(d_tests_draw() * _scale);
            }
        }
    }

    template<typename Type>
    void
    d_tests_fill(
        la::dynamic_matrix<Type>& _m,
        double                    _scale
    )
    {
        for (std::size_t i = 0; i < _m.rows(); ++i)
        {
            for (std::size_t j = 0; j < _m.cols(); ++j)
            {
                _m(i, j) = static_cast<Type>(d_tests_draw() * _scale);
            }
        }
    }

    // d_tests_types
    //   what the lazy nodes are and evaluate to.
    void
    d_tests_types()
    {
        la::vector<double, 3>      v;
        la::matrix<double, 2, 3>   m;
        la::matrix<double, 3, 2>   n;
        la::dynamic_matrix<double> d(2, 3);

        const bool lazy_ones =
            la::is_lazy<decltype(la::lazy(v))>::value &&
            la::is_lazy<decltype(la::lazy(v) * 2.0 + la::lazy(v))>::value &&
            (!la::is_lazy<la::vector<double, 3> >::value);
        const bool evaluated =
            std::is_same<decltype(la::eval(la::lazy(v) + la::lazy(v))),
                         la::vector<double, 3> >::value &&
            std::is_same<decltype(la::eval(la::lazy(m) * la::lazy(n))),
                         la::matrix<double, 2, 2> >::value &&
            std::is_same<decltype(la::eval(la::lazy(m) * la::lazy(v))),
                         la::vector<double, 2> >::value &&
            std::is_same<decltype(la::eval(la::transpose(la::lazy(m)))),
                         la::matrix<double, 3, 2> >::value &&
            std::is_same<decltype(la::eval(la::lazy(d) * 3)),
                         la::dynamic_matrix<double> >::value;

        D_TESTS_EXPECT( lazy_ones,
                        "lazy(x) and what its operators build are lazy; "
                        "a vector is not");
        D_TESTS_EXPECT( evaluated,
                        "each evaluates to the type eager arithmetic gives");
        D_TESTS_EXPECT( (la::lazy(d).rows() == 2) &&
                        (la::lazy(d).cols() == 3) &&
                        (la::lazy(v).rows() == 3) &&
                        (la::lazy(v).cols() == 1),
                        "a leaf's shape");

        return;
    }

    // d_tests_vectors
    //   vectors of double, float, long double and complex numbers.
    void
    d_tests_vectors()
    {
        bool dbl = true;
        bool flt = true;
        bool ldb = true;
        bool cpx = true;

        // 300 draws
        for (int t = 0; t < 300; ++t)
        {
            la::vector<double, 7> a;
            la::vector<double, 7> b;
            la::vector<double, 7> c;

            d_tests_fill(a, 1.0e3);
            d_tests_fill(b, 1.0);
            d_tests_fill(c, 1.0e-3);

            const double k = d_tests_draw() * 3.7;
            const double q = d_tests_draw() + 2.0;

            dbl = dbl &&
                  d_tests_same_vectors(la::eval(la::lazy(a) * k +
                                                la::lazy(b) - la::lazy(c)),
                                       a * k + b - c) &&
                  d_tests_same_vectors((-(la::lazy(a) - la::lazy(b) * q) +
                                        la::lazy(c) / k).eval(),
                                       -(a - b * q) + c / k) &&
                  d_tests_same_vectors(la::eval(la::hadamard(la::lazy(a),
                                                             la::lazy(b)) +
                                                q * la::lazy(c)),
                                       a.hadamard(b) + q * c);

            la::vector<float, 5> fa;
            la::vector<float, 5> fb;

            d_tests_fill(fa, 10.0);
            d_tests_fill(fb, 1.0);
            flt = flt &&
                  d_tests_same_vectors(la::eval(la::lazy(fa) * k +
                                                la::lazy(fb)),
                                       fa * k + fb);

            la::vector<long double, 4> la_;
            la::vector<long double, 4> lb;

            d_tests_fill(la_, 10.0);
            d_tests_fill(lb, 1.0);
            ldb = ldb &&
                  d_tests_same_vectors(la::eval(la::lazy(la_) * q -
                                                la::lazy(lb) / k),
                                       la_ * q - lb / k);

            la::vector<cd, 3> za;
            la::vector<cd, 3> zb;

            d_tests_fill(za, 1.0);
            d_tests_fill(zb, 2.0);
            cpx = cpx &&
                  d_tests_same_vectors(la::eval(la::lazy(za) * k +
                                                la::lazy(zb)),
                                       za * k + zb) &&
                  d_tests_same_vectors(la::eval(la::hadamard(la::lazy(za),
                                                             la::lazy(zb)) -
                                                la::lazy(za)),
                                       za.hadamard(zb) - za);
        }

    #if D_TESTS_FIXED_BITS
        D_TESTS_EXPECT( dbl,
                        "vectors of double: sums, differences, negations, "
                        "scalings, a division and entry-wise products, the "
                        "eager bits, at 300 draws");
        D_TESTS_EXPECT( flt && ldb,
                        "vectors of float and long double, the eager bits, "
                        "at 300 draws");
        D_TESTS_EXPECT( cpx,
                        "vectors of complex numbers, the eager bits, at 300 "
                        "draws");
    #else
        static_cast<void>(dbl);
        static_cast<void>(flt);
        static_cast<void>(ldb);
        static_cast<void>(cpx);
    #endif  // D_TESTS_FIXED_BITS

        return;
    }

    // d_tests_matrices
    //   fixed-size matrices: element-wise, products and transposes.
    void
    d_tests_matrices()
    {
        bool same = true;

        // 300 draws
        for (int t = 0; t < 300; ++t)
        {
            la::matrix<double, 3, 4> a;
            la::matrix<double, 3, 4> b;
            la::matrix<double, 4, 3> c;
            la::vector<double, 4>    v;

            d_tests_fill(a, 1.0);
            d_tests_fill(b, 10.0);
            d_tests_fill(c, 1.0);
            d_tests_fill(v, 100.0);

            const double k = d_tests_draw() * 3.7;
            const double q = d_tests_draw() + 2.0;

            same = same &&
                   d_tests_same_matrices(la::eval(la::lazy(a) * la::lazy(c) *
                                                  k +
                                                  (la::lazy(a) - la::lazy(b)) *
                                                  la::lazy(c)),
                                         a * c * k + (a - b) * c) &&
                   d_tests_same_matrices(la::eval(la::transpose(la::lazy(a)) *
                                                  q + la::lazy(c)),
                                         a.transposed() * q + c) &&
                   d_tests_same_vectors(la::eval(la::lazy(a) * la::lazy(v) -
                                                 la::lazy(b) * la::lazy(v)),
                                        a * v - b * v);
        }

    #if D_TESTS_FIXED_BITS
        D_TESTS_EXPECT( same,
                        "fixed-size matrices: element-wise, products by a "
                        "matrix and by a vector, a transpose, the eager bits, "
                        "at 300 draws");
    #else
        static_cast<void>(same);
    #endif  // D_TESTS_FIXED_BITS

        return;
    }

    // d_tests_dynamic
    //   dynamic matrices: element-wise, products, transposes, assign.
    void
    d_tests_dynamic()
    {
        bool elements = true;
        bool products = true;
        bool assigned = true;

        // 300 draws
        for (int t = 0; t < 300; ++t)
        {
            la::dynamic_matrix<double> a(5, 6);
            la::dynamic_matrix<double> b(5, 6);
            la::dynamic_matrix<double> c(6, 5);

            d_tests_fill(a, 1.0);
            d_tests_fill(b, 7.0);
            d_tests_fill(c, 1.0);

            const double k = d_tests_draw() * 3.7;
            const double q = d_tests_draw() + 2.0;

            elements = elements &&
                       d_tests_same_dynamic(la::eval(la::lazy(a) * k +
                                                     la::lazy(b) * q -
                                                     la::lazy(a)),
                                            a * k + b * q - a) &&
                       d_tests_same_dynamic(la::eval(-la::hadamard(
                                                la::lazy(a), la::lazy(b))),
                                            -(a.hadamard(b)));
            products = products &&
                       d_tests_same_dynamic(la::eval(la::lazy(a) *
                                                     la::lazy(c) * k +
                                                     la::lazy(a) *
                                                     la::lazy(c)),
                                            (a * c) * k + (a * c)) &&
                       d_tests_same_dynamic(la::eval(la::transpose(
                                                la::lazy(a)) - la::lazy(c)),
                                            a.transposed() - c);

            // in place, the destination read; into another shape
            la::dynamic_matrix<double> g = a;
            la::dynamic_matrix<double> h(2, 2);

            la::assign(g, la::lazy(g) * k + la::lazy(b));
            (la::transpose(la::lazy(a)) * q).assign_to(h);
            assigned = assigned &&
                       d_tests_same_dynamic(g, a * k + b) &&
                       d_tests_same_dynamic(h, a.transposed() * q);
        }

        D_TESTS_EXPECT( elements,
                        "dynamic matrices: element-wise, the eager bits, at "
                        "300 draws");
        D_TESTS_EXPECT( products,
                        "dynamic matrices: products and a transpose, the "
                        "eager bits, at 300 draws");
        D_TESTS_EXPECT( assigned,
                        "assign in place, the destination read, and into "
                        "another shape, the eager bits, at 300 draws");

        return;
    }

    // d_tests_assign_fixed
    //   assign into a fixed-size value, the destination read.
    void
    d_tests_assign_fixed()
    {
        la::matrix<double, 3, 3> a;
        la::matrix<double, 3, 3> b;

        d_tests_fill(a, 1.0);
        d_tests_fill(b, 1.0);

        la::matrix<double, 3, 3>       g = a;
        const la::matrix<double, 3, 3> expected = a * b + a * 0.5;

        la::assign(g, la::lazy(g) * la::lazy(b) + la::lazy(g) * 0.5);

    #if D_TESTS_FIXED_BITS
        D_TESTS_EXPECT( d_tests_same_matrices(g, expected),
                        "assign into a fixed-size matrix it reads, through a "
                        "product, the eager bits");
    #else
        static_cast<void>(expected);
    #endif  // D_TESTS_FIXED_BITS

        return;
    }

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT( DJINTERP_MATH_LINEAR_ALGEBRA_LAZY_HPP == 1,
                    "lazy.hpp reads at every level, empty below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_types();
    d_tests_vectors();
    d_tests_matrices();
    d_tests_dynamic();
    d_tests_assign_fixed();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    std::printf("t_lazy_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
