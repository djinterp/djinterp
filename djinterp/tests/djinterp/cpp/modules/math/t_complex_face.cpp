/*******************************************************************************
* djinterp [cpp/math]                                         t_complex_face.cpp
*
*   Tests for math/complex.hpp, math::complex<T> (S270), at every level from
* C++98, per family, float, double and long double: construction, from two
* parts, a real number, a std::complex<T>, the C core's struct and another
* part type, and the parts, c_value() and to_std() out; every operator, a
* real operand on either side, and its assignment, cplx.h's bit for bit on a
* grid of special parts -- zeros of both signs, ones, the smallest normal,
* the largest finite, the infinities and a NaN -- and on random points, a
* real operand taken as itself (a zero's sign kept); == and != part by
* part; real, imag, conj and proj; abs, arg and the 21 functions, exp2, log2,
* log10, log1p and expm1 among them, each cplx.h's _c form bit for bit, never
* its plain form, and at known points their exact values; number_traits, a field
* with no order, its magnitude |re| + |im|, and common_number; and linear
* algebra of it: sums and products, the bilinear dot and the conjugating vdot,
* the norms of the moduli and is_unitary (S354), tolerances of the real type,
* and the determinant, inverse and solution of random matrices of 1 to 5 rows,
* cmat.h's bit for bit. From C++14, every function as a constant
* expression, equal to the run time's bit for bit, and a determinant as
* one.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/math/t_complex_face.cpp -o t_complex -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_complex_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.11.02
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/complex.hpp"  // tested
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/cmat.h"  // d_cmat_*: the references
#include "../../../../../inc/djinterp/cpp/modules/math/linear_algebra/solve.hpp"  // solve
#include "../../../../../inc/djinterp/cpp/modules/math/linear_algebra/square.hpp"  // det, inv
// std
#include <complex>  // std::complex
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <limits>   // std::numeric_limits


namespace
{
    namespace m  = djinterp::math;
    namespace la = djinterp::math::linalg;

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

    // d_tests_same_type
    //   1 where A and B are one type, at every level.
    template<typename A,
             typename B>
    struct d_tests_same_type
    {
        enum
        {
            value = 0
        };
    };

    template<typename A>
    struct d_tests_same_type<A, A>
    {
        enum
        {
            value = 1
        };
    };

    // d_tests_negative
    //   whether _x's sign is negative: below 0, or a zero whose reciprocal
    // is -inf.
    template<typename T>
    bool
    d_tests_negative(T _x)
    {
        return ( (_x < T(0)) ||
                 ( (_x == T(0)) &&
                   ((T(1) / _x) < T(0)) ) );
    }

    // d_tests_same_real
    //   two numbers the same once stored, so that the x87's excess
    // precision does not tell them apart: equal with one sign, or both NaN.
    template<typename T>
    bool
    d_tests_same_real(T _a, T _b)
    {
        volatile T a = _a;
        volatile T b = _b;
        const T    x = a;
        const T    y = b;

        if (!(x == x))
        {
            return !(y == y);
        }

        return ( (x == y) &&
                 (d_tests_negative(x) == d_tests_negative(y)) );
    }

    // d_tests_same
    //   a face's complex number and the core's struct of the same parts,
    // bit for bit as d_tests_same_real has them.
    template<typename T>
    bool
    d_tests_same(
        const m::complex<T>&                     _z,
        const typename m::complex<T>::core_type& _w
    )
    {
        return ( (d_tests_same_real(_z.real(), _w.part[0])) &&
                 (d_tests_same_real(_z.imag(), _w.part[1])) );
    }

    template<typename T>
    bool
    d_tests_same(
        const m::complex<T>& _z,
        const m::complex<T>& _w
    )
    {
        return d_tests_same(_z, _w.c_value());
    }

    // a value in [-1, 1) from a 32-bit linear congruential generator
    unsigned long g_state = 20261030UL;

    double
    d_tests_random()
    {
        g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

        return (static_cast<double>(g_state >> 8) / 8388608.0) - 1.0;
    }

    // d_tests_specials
    //   the parts of the grid: zeros of both signs, ones, a half, 3, the
    // smallest normal and a multiple, the largest finite and its half, the
    // infinities and a NaN; 14 of them.
    template<typename T>
    void
    d_tests_specials(T* _out)
    {
        typedef std::numeric_limits<T> limits;

        _out[0]  = T(0);
        _out[1]  = -T(0);
        _out[2]  = T(1);
        _out[3]  = T(-1);
        _out[4]  = T(0.5);
        _out[5]  = T(3);
        _out[6]  = limits::min();
        _out[7]  = -(limits::min() * T(3));
        _out[8]  = limits::max() / T(2);
        _out[9]  = -limits::max();
        _out[10] = limits::infinity();
        _out[11] = -limits::infinity();
        _out[12] = limits::quiet_NaN();
        _out[13] = T(-2.5);

        return;
    }

    // d_tests_points
    //   the points the operations are checked at: the grid's 196, then
    // random points of every scale, _count in all.
    template<typename T>
    void
    d_tests_points(
        m::complex<T>* _out,
        std::size_t    _count
    )
    {
        T           special[14];
        std::size_t k = 0;

        d_tests_specials(special);

        for (std::size_t i = 0; ( (i < 14) && (k < _count) ); ++i)
        {
            for (std::size_t j = 0; ( (j < 14) && (k < _count) ); ++j)
            {
                _out[k++] = m::complex<T>(special[i], special[j]);
            }
        }

        while (k < _count)
        {
            const double scale = (k % 3 == 0) ? 1.0
                               : (k % 3 == 1) ? 1e-3
                                              : 40.0;

            _out[k++] = m::complex<T>(
                static_cast<T>(d_tests_random() * scale),
                static_cast<T>(d_tests_random() * scale));
        }

        return;
    }

    // d_tests_construction
    //   construction and the parts.
    template<typename T>
    void
    d_tests_construction()
    {
        typedef m::complex<T>                  C;
        typedef typename C::core_type          K;
        typedef std::complex<T>                S;

        const C z0;
        const C z1(T(2));
        const C z2(T(-1.5), T(0.25));
        const C zs   = S(T(3), -T(0));
        const K core = d_cplx_make(T(5), T(-7));
        const C zk(core);
        const C zf(m::complex<float>(1.5f, -0.0f));
        C       set(T(1), T(2));

        set.real(T(-4));
        set.imag(-T(0));

        D_TESTS_EXPECT( (d_tests_same_type<typename C::value_type,
                                           T>::value == 1) &&
                        (d_tests_same(z0, d_cplx_make(T(0), T(0)))) &&
                        (d_tests_same(z1, d_cplx_make(T(2), T(0)))) &&
                        (d_tests_same(z2, d_cplx_make(T(-1.5), T(0.25)))) &&
                        (d_tests_same(zs, d_cplx_make(T(3), -T(0)))) &&
                        (d_tests_same(zk, core)) &&
                        (d_tests_same(zf, d_cplx_make(T(1.5), -T(0)))) &&
                        (d_tests_same(set, d_cplx_make(T(-4), -T(0)))),
                        "construction: 0, a real number with +0, two parts, "
                        "a std::complex, the core's struct, another type");

        const S out = z2.to_std();
        const C two = C(S(T(1), T(1))) + S(T(1), T(1));

        D_TESTS_EXPECT( (d_tests_same_real(out.real(), T(-1.5))) &&
                        (d_tests_same_real(out.imag(), T(0.25))) &&
                        (d_tests_same_real(zs.to_std().imag(), -T(0))) &&
                        (d_tests_same(two, d_cplx_make(T(2), T(2)))) &&
                        (d_tests_same_real(real(z2), T(-1.5))) &&
                        (d_tests_same_real(imag(z2), T(0.25))) &&
                        (d_tests_same_real(z2.c_value().part[0],
                                           T(-1.5))) &&
                        (sizeof(C) == sizeof(K)),
                        "the parts, the core's struct lent, to_std, a "
                        "std::complex operand taken as this type");

        return;
    }

    // d_tests_arithmetic
    //   every operator against cplx.h's arithmetic, bit for bit, at _n
    // points, each with every other of the first 40 and a real part of each.
    template<typename T>
    void
    d_tests_arithmetic(
        const m::complex<T>* _p,
        std::size_t          _n
    )
    {
        typedef m::complex<T>         C;
        typedef typename C::core_type K;

        bool unary = true;
        bool sums  = true;
        bool prods = true;
        bool quots = true;
        bool reals = true;
        bool sets  = true;
        bool eqs   = true;

        for (std::size_t i = 0; i < _n; ++i)
        {
            const C z  = _p[i];
            const K zc = z.c_value();

            unary = ( (unary) &&
                      (d_tests_same(-z, d_cplx_neg(zc))) &&
                      (d_tests_same(+z, zc)) );

            for (std::size_t j = 0; j < 40; ++j)
            {
                const C w  = _p[j];
                const K wc = w.c_value();
                const T x  = w.real();
                C       t  = z;

                sums  = ( (sums) &&
                          (d_tests_same(z + w, d_cplx_add(zc, wc))) &&
                          (d_tests_same(z - w, d_cplx_sub(zc, wc))) );
                prods = ( (prods) &&
                          (d_tests_same(z * w, d_cplx_mul(zc, wc))) );
                quots = ( (quots) &&
                          (d_tests_same(z / w, d_cplx_div(zc, wc))) );
                reals = ( (reals) &&
                          (d_tests_same(z + x, d_cplx_add_real(zc, x))) &&
                          (d_tests_same(x + z, d_cplx_add_real(zc, x))) &&
                          (d_tests_same(z - x, d_cplx_sub_real(zc, x))) &&
                          (d_tests_same(x - z, d_cplx_real_sub(x, zc))) &&
                          (d_tests_same(z * x, d_cplx_mul_real(zc, x))) &&
                          (d_tests_same(x * z, d_cplx_mul_real(zc, x))) &&
                          (d_tests_same(z / x, d_cplx_div_real(zc, x))) &&
                          (d_tests_same(x / z, d_cplx_real_div(x, zc))) );

                t += w;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_add(zc, wc))) );
                t = z;
                t -= w;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_sub(zc, wc))) );
                t = z;
                t *= w;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_mul(zc, wc))) );
                t = z;
                t /= w;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_div(zc, wc))) );
                t = z;
                t += x;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_add_real(zc, x))) );
                t = z;
                t -= x;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_sub_real(zc, x))) );
                t = z;
                t *= x;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_mul_real(zc, x))) );
                t = z;
                t /= x;
                sets = ( (sets) &&
                         (d_tests_same(t, d_cplx_div_real(zc, x))) );

                eqs = ( (eqs) &&
                        ((z == w) == d_cplx_equal(zc, wc)) &&
                        ((z != w) == !d_cplx_equal(zc, wc)) &&
                        ((z == x) == d_cplx_equal(zc,
                                                  d_cplx_make(x, T(0)))) &&
                        ((x == z) == (z == x)) &&
                        ((z != x) == !(z == x)) &&
                        ((x != z) == !(z == x)) );
            }
        }

        D_TESTS_EXPECT(unary, "-z and +z: cplx.h's");
        D_TESTS_EXPECT(sums, "z + w and z - w: cplx.h's, bit for bit");
        D_TESTS_EXPECT(prods, "z w: cplx.h's, bit for bit");
        D_TESTS_EXPECT(quots, "z / w: cplx.h's, bit for bit");
        D_TESTS_EXPECT(reals, "a real operand on either side: cplx.h's");
        D_TESTS_EXPECT(sets, "the assignments: the operators' bits");
        D_TESTS_EXPECT(eqs, "== and !=: cplx.h's equality, part by part");

        // a real operand as itself (G.5): (1 + 0i) (-1) is -1 - 0i, where
        // (1 + 0i) (-1 + 0i) is -1 + 0i; -0 == 0 and NaN != NaN
        const T nan = std::numeric_limits<T>::quiet_NaN();

        D_TESTS_EXPECT( (d_tests_same(C(T(1), T(0)) * T(-1),
                                      d_cplx_make(T(-1), -T(0)))) &&
                        (d_tests_same(C(T(1), T(0)) * C(T(-1), T(0)),
                                      d_cplx_make(T(-1), T(0)))) &&
                        (C(-T(0), T(0)) == C(T(0), -T(0))) &&
                        (C(T(2), -T(0)) == T(2)) &&
                        (!(C(nan, T(0)) == C(nan, T(0)))) &&
                        (C(T(1), nan) != T(1)),
                        "a real operand as itself; -0 == 0; NaN != NaN");

        return;
    }

    // d_tests_function
    //   whether _face, a function of the face, gives at each of _n points
    // what _core, cplx.h's _c form, does, bit for bit; and how many points
    // the plain form _plain gives other bits at, in *_apart.
    template<typename T,
             typename Face,
             typename Core>
    bool
    d_tests_function(
        const m::complex<T>* _p,
        std::size_t          _n,
        Face                 _face,
        Core                 _core,
        Core                 _plain,
        long*                _apart
    )
    {
        bool ok = true;

        for (std::size_t i = 0; i < _n; ++i)
        {
            const typename m::complex<T>::core_type want =
                _core(_p[i].c_value());

            ok = ( (ok) &&
                   (d_tests_same(_face(_p[i]), want)) );

            if (!d_tests_same(m::complex<T>(_plain(_p[i].c_value())), want))
            {
                ++(*_apart);
            }
        }

        return ok;
    }

    // the face's functions, and the core's, as plain function pointers'
    // targets: each found by argument-dependent lookup, as a caller finds
    // it
    template<typename T>
    struct d_tests_calls
    {
        typedef m::complex<T>         C;
        typedef typename C::core_type K;

        static C f_exp(C _z)   { return exp(_z); }
        static C f_log(C _z)   { return log(_z); }
        static C f_sqrt(C _z)  { return sqrt(_z); }
        static C f_sin(C _z)   { return sin(_z); }
        static C f_cos(C _z)   { return cos(_z); }
        static C f_tan(C _z)   { return tan(_z); }
        static C f_asin(C _z)  { return asin(_z); }
        static C f_acos(C _z)  { return acos(_z); }
        static C f_atan(C _z)  { return atan(_z); }
        static C f_sinh(C _z)  { return sinh(_z); }
        static C f_cosh(C _z)  { return cosh(_z); }
        static C f_tanh(C _z)  { return tanh(_z); }
        static C f_asinh(C _z) { return asinh(_z); }
        static C f_acosh(C _z) { return acosh(_z); }
        static C f_atanh(C _z) { return atanh(_z); }
        static C f_exp2(C _z)  { return exp2(_z); }
        static C f_log2(C _z)  { return log2(_z); }
        static C f_log10(C _z) { return log10(_z); }
        static C f_log1p(C _z) { return log1p(_z); }
        static C f_expm1(C _z) { return expm1(_z); }
        static C f_conj(C _z)  { return conj(_z); }
        static C f_proj(C _z)  { return proj(_z); }
        static C f_abs(C _z)   { return C(abs(_z), T(0)); }
        static C f_arg(C _z)   { return C(arg(_z), T(0)); }

        static K c_exp(K _z)   { return d_cplx_exp_c(_z); }
        static K c_log(K _z)   { return d_cplx_log_c(_z); }
        static K c_sqrt(K _z)  { return d_cplx_sqrt_c(_z); }
        static K c_sin(K _z)   { return d_cplx_sin_c(_z); }
        static K c_cos(K _z)   { return d_cplx_cos_c(_z); }
        static K c_tan(K _z)   { return d_cplx_tan_c(_z); }
        static K c_asin(K _z)  { return d_cplx_asin_c(_z); }
        static K c_acos(K _z)  { return d_cplx_acos_c(_z); }
        static K c_atan(K _z)  { return d_cplx_atan_c(_z); }
        static K c_sinh(K _z)  { return d_cplx_sinh_c(_z); }
        static K c_cosh(K _z)  { return d_cplx_cosh_c(_z); }
        static K c_tanh(K _z)  { return d_cplx_tanh_c(_z); }
        static K c_asinh(K _z) { return d_cplx_asinh_c(_z); }
        static K c_acosh(K _z) { return d_cplx_acosh_c(_z); }
        static K c_atanh(K _z) { return d_cplx_atanh_c(_z); }
        static K c_exp2(K _z)  { return d_cplx_exp2_c(_z); }
        static K c_log2(K _z)  { return d_cplx_log2_c(_z); }
        static K c_log10(K _z) { return d_cplx_log10_c(_z); }
        static K c_log1p(K _z) { return d_cplx_log1p_c(_z); }
        static K c_expm1(K _z) { return d_cplx_expm1_c(_z); }
        static K c_conj(K _z)  { return d_cplx_conj(_z); }
        static K c_proj(K _z)  { return d_cplx_proj(_z); }
        static K c_abs(K _z)   { return d_cplx_make(d_cplx_abs_c(_z), T(0)); }
        static K c_arg(K _z)   { return d_cplx_make(d_cplx_arg_c(_z), T(0)); }

        static K p_exp(K _z)   { return d_cplx_exp(_z); }
        static K p_log(K _z)   { return d_cplx_log(_z); }
        static K p_sqrt(K _z)  { return d_cplx_sqrt(_z); }
        static K p_sin(K _z)   { return d_cplx_sin(_z); }
        static K p_cos(K _z)   { return d_cplx_cos(_z); }
        static K p_tan(K _z)   { return d_cplx_tan(_z); }
        static K p_asin(K _z)  { return d_cplx_asin(_z); }
        static K p_acos(K _z)  { return d_cplx_acos(_z); }
        static K p_atan(K _z)  { return d_cplx_atan(_z); }
        static K p_sinh(K _z)  { return d_cplx_sinh(_z); }
        static K p_cosh(K _z)  { return d_cplx_cosh(_z); }
        static K p_tanh(K _z)  { return d_cplx_tanh(_z); }
        static K p_asinh(K _z) { return d_cplx_asinh(_z); }
        static K p_acosh(K _z) { return d_cplx_acosh(_z); }
        static K p_atanh(K _z) { return d_cplx_atanh(_z); }
        static K p_exp2(K _z)  { return d_cplx_exp2(_z); }
        static K p_log2(K _z)  { return d_cplx_log2(_z); }
        static K p_log10(K _z) { return d_cplx_log10(_z); }
        static K p_log1p(K _z) { return d_cplx_log1p(_z); }
        static K p_expm1(K _z) { return d_cplx_expm1(_z); }
        static K p_abs(K _z)   { return d_cplx_make(d_cplx_abs(_z), T(0)); }
        static K p_arg(K _z)   { return d_cplx_make(d_cplx_arg(_z), T(0)); }
    };

    // d_tests_functions
    //   every function against cplx.h's _c form, bit for bit, at _n points;
    // pow of two complex numbers, and of a real operand as it plus 0i, at
    // the first 40 by the first 40; and how many points each plain form
    // gives other bits at, which the face's must not be.
    template<typename T>
    void
    d_tests_functions(
        const m::complex<T>* _p,
        std::size_t          _n
    )
    {
        typedef d_tests_calls<T> F;
        typedef m::complex<T>    C;

        long apart = 0;
        bool exact = true;

        exact = d_tests_function<T>(_p, _n, &F::f_conj, &F::c_conj,
                                    &F::c_conj, &apart) && exact;
        exact = d_tests_function<T>(_p, _n, &F::f_proj, &F::c_proj,
                                    &F::c_proj, &apart) && exact;

        D_TESTS_EXPECT(exact, "conj and proj: cplx.h's");

        bool ok = true;

        apart = 0;
        ok = d_tests_function<T>(_p, _n, &F::f_abs, &F::c_abs, &F::p_abs,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_arg, &F::c_arg, &F::p_arg,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_exp, &F::c_exp, &F::p_exp,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_log, &F::c_log, &F::p_log,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_sqrt, &F::c_sqrt, &F::p_sqrt,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_sin, &F::c_sin, &F::p_sin,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_cos, &F::c_cos, &F::p_cos,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_tan, &F::c_tan, &F::p_tan,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_asin, &F::c_asin, &F::p_asin,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_acos, &F::c_acos, &F::p_acos,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_atan, &F::c_atan, &F::p_atan,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_sinh, &F::c_sinh, &F::p_sinh,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_cosh, &F::c_cosh, &F::p_cosh,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_tanh, &F::c_tanh, &F::p_tanh,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_asinh, &F::c_asinh,
                                 &F::p_asinh, &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_acosh, &F::c_acosh,
                                 &F::p_acosh, &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_atanh, &F::c_atanh,
                                 &F::p_atanh, &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_exp2, &F::c_exp2, &F::p_exp2,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_log2, &F::c_log2, &F::p_log2,
                                 &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_log10, &F::c_log10,
                                 &F::p_log10, &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_log1p, &F::c_log1p,
                                 &F::p_log1p, &apart) && ok;
        ok = d_tests_function<T>(_p, _n, &F::f_expm1, &F::c_expm1,
                                 &F::p_expm1, &apart) && ok;

        D_TESTS_EXPECT(ok, "abs, arg and the 21 functions: cplx.h's _c "
                           "forms, bit for bit");
        D_TESTS_EXPECT(apart > 0, "the plain forms give other bits at some "
                                  "points, which the face's never do");

        // pow at 80 points by 80: every fifth of the grid's, and 40 random
        // ones; each by a complex exponent, by a real one, and a real base,
        // real z among them, where the plain form takes the real pow
        bool pows = true;
        C    sample[80];

        for (std::size_t k = 0; k < 40; ++k)
        {
            sample[k]      = _p[k * 5];
            sample[40 + k] = _p[196 + k];
        }

        for (std::size_t i = 0; i < 80; ++i)
        {
            for (std::size_t j = 0; j < 80; ++j)
            {
                const C z  = sample[i];
                const C w  = sample[j];
                const T x  = w.real();
                const C zr = C(z.real());

                pows = ( (pows) &&
                         (d_tests_same(pow(z, w),
                                       d_cplx_pow_c(z.c_value(),
                                                    w.c_value()))) &&
                         (d_tests_same(pow(z, x),
                                       d_cplx_pow_c(z.c_value(),
                                                    d_cplx_make(x, T(0))))) &&
                         (d_tests_same(pow(zr, x),
                                       d_cplx_pow_c(zr.c_value(),
                                                    d_cplx_make(x, T(0))))) &&
                         (d_tests_same(pow(x, z),
                                       d_cplx_pow_c(d_cplx_make(x, T(0)),
                                                    z.c_value()))) );
            }
        }

        D_TESTS_EXPECT(pows, "pow: cplx.h's _c form, never its plain one; a "
                             "real operand as it plus 0i");

        // exact values: |3 + 4i| 5, sqrt(2i) 1 + i, sqrt(-4) 2i, exp(0) 1,
        // log(1) 0, arg(-1 + 0i) and arg(-1 - 0i) pi and -pi, (2i)^2 -4
        const T pi = T(3.14159265358979323846264338327950288L);

        D_TESTS_EXPECT( (abs(C(T(3), T(4))) == T(5)) &&
                        (d_tests_same(sqrt(C(T(0), T(2))),
                                      d_cplx_make(T(1), T(1)))) &&
                        (d_tests_same(sqrt(C(T(-4), T(0))),
                                      d_cplx_make(T(0), T(2)))) &&
                        (d_tests_same(sqrt(C(T(-4), -T(0))),
                                      d_cplx_make(T(0), T(-2)))) &&
                        (d_tests_same(exp(C()), d_cplx_make(T(1), T(0)))) &&
                        (d_tests_same(log(C(T(1))),
                                      d_cplx_make(T(0), T(0)))) &&
                        (arg(C(T(-1), T(0))) == pi) &&
                        (arg(C(T(-1), -T(0))) == -pi) &&
                        (C(T(0), T(2)) * C(T(0), T(2)) == T(-4)),
                        "exact values, the cuts' signs among them");

        // the five: log2(-8) 3 + i pi/ln 2, its real part exactly 3;
        // log2(8), log10(100) and exp2(3) the real values; log1p(-2) 0 + i
        // pi; expm1(0) 0
        D_TESTS_EXPECT( (log2(C(T(-8))).real() == T(3)) &&
                        (log2(C(T(-8))).imag() > T(4.53)) &&
                        (log2(C(T(-8))).imag() < T(4.54)) &&
                        (d_tests_same(log2(C(T(8))),
                                      d_cplx_make(T(3), T(0)))) &&
                        (d_tests_same(log10(C(T(100), -T(0))),
                                      d_cplx_make(T(2), -T(0)))) &&
                        (d_tests_same(exp2(C(T(3))),
                                      d_cplx_make(T(8), T(0)))) &&
                        (log1p(C(T(-2))).real() == T(0)) &&
                        (log1p(C(T(-2))).imag() == pi) &&
                        (d_tests_same(expm1(C()), d_cplx_make(T(0), T(0)))),
                        "the five's exact values");

        return;
    }

    // d_tests_numbers
    //   number_traits and common_number.
    template<typename T>
    void
    d_tests_numbers()
    {
        typedef m::complex<T>            C;
        typedef m::number_traits<C>      traits;
        typedef std::numeric_limits<T>   limits;

        D_TESTS_EXPECT( (traits::is_specialized) &&
                        (!traits::is_exact) &&
                        (!traits::is_integer) &&
                        (traits::is_field) &&
                        (traits::is_commutative) &&
                        (!traits::is_ordered) &&
                        (d_tests_same_type<typename traits::magnitude_type,
                                           T>::value == 1),
                        "number_traits: a commutative field, inexact, with "
                        "no order, its magnitude a T");
        D_TESTS_EXPECT( (d_tests_same(traits::zero(),
                                      d_cplx_make(T(0), T(0)))) &&
                        (d_tests_same(traits::one(),
                                      d_cplx_make(T(1), T(0)))) &&
                        (traits::magnitude(C(T(-3), T(4))) == T(7)) &&
                        (traits::magnitude(C(-T(0), -T(0))) == T(0)) &&
                        (traits::epsilon() == limits::epsilon()) &&
                        (traits::epsilon(C(T(9))) == limits::epsilon()) &&
                        (traits::tolerance() == T(4) * limits::epsilon()) &&
                        (traits::tolerance(C(T(1), T(1)), C(T(-3), T(4))) ==
                         T(4) * limits::epsilon() * T(7)),
                        "zero, one, the magnitude |re| + |im|, epsilon and "
                        "the tolerances, of T");
        D_TESTS_EXPECT( (d_tests_same_type<
                             typename m::common_number<C, C>::type,
                             C>::value == 1) &&
                        (d_tests_same_type<
                             typename m::common_number<C, T>::type,
                             C>::value == 1) &&
                        (d_tests_same_type<
                             typename m::common_number<T, C>::type,
                             C>::value == 1),
                        "common_number: the complex type");

        return;
    }

    // d_tests_matrix_parity
    //   _trials random N-by-N matrices' det, inverse and solution of two
    // columns, cmat.h's bit for bit, every fifth of small integers, which
    // make zero pivots, ties and singular matrices; and A inverse(A) the
    // identity, where A is well away from singular.
    template<typename    T,
             std::size_t N>
    bool
    d_tests_matrix_parity(int _trials)
    {
        typedef m::complex<T>         C;
        typedef typename C::core_type K;

        bool ok = true;

        for (int t = 0; t < _trials; ++t)
        {
            la::matrix<C, N, N> a;
            la::matrix<C, N, 2> b;
            K                   ka[N * N];
            K                   kb[N * 2];
            K                   ks[N * N];
            K                   kx[N * 2];
            K                   ko[N * N];

            for (std::size_t i = 0; i < (N * N); ++i)
            {
                T re = static_cast<T>(d_tests_random());
                T im = static_cast<T>(d_tests_random());

                if ((t % 5) == 0)
                {
                    re = static_cast<T>(static_cast<int>(re * T(3)));
                    im = static_cast<T>(static_cast<int>(im * T(2)));
                }

                a(i / N, i % N) = C(re, im);
                ka[i]           = a(i / N, i % N).c_value();
            }

            for (std::size_t i = 0; i < (N * 2); ++i)
            {
                b(i / 2, i % 2) = C(static_cast<T>(d_tests_random()),
                                    static_cast<T>(d_tests_random()));
                kb[i]           = b(i / 2, i % 2).c_value();
            }

            const C                   d  = la::determinant(a);
            const K                   kd = d_cmat_determinant(ka, ks, N);
            const la::matrix<C, N, N> ia = la::inverse(a);
            const bool                ki = d_cmat_inverse(ko, ks, ka, N);
            const la::matrix<C, N, 2> x  = la::solve(a, b);
            const bool                kx_ok = d_cmat_solve(kx, ks, ka, kb,
                                                           N, 2);

            ok = ( (ok) &&
                   (d_tests_same(d, kd)) );

            for (std::size_t i = 0; i < (N * N); ++i)
            {
                ok = ( (ok) &&
                       (d_tests_same(ia(i / N, i % N), ko[i])) );
            }

            for (std::size_t i = 0; i < (N * 2); ++i)
            {
                ok = ( (ok) &&
                       (d_tests_same(x(i / 2, i % 2), kx[i])) );
            }

            // A well away from singular: A A^-1 the identity, within a few
            // thousand epsilons
            if ( (ki) &&
                 (kx_ok) &&
                 (la::is_invertible(a, T(0.25))) )
            {
                ok = ( (ok) &&
                       (a.times(ia).equals(
                            la::matrix<C, N, N>::identity(),
                            T(4096) * std::numeric_limits<T>::epsilon())) );
            }
        }

        return ok;
    }

    // d_tests_matrices
    //   linear algebra of complex numbers: the parity above, for 1 to 5
    // rows; sums, products and the bilinear dot; tolerances of T.
    template<typename T>
    void
    d_tests_matrices()
    {
        typedef m::complex<T> C;

        D_TESTS_EXPECT( (d_tests_matrix_parity<T, 1>(100)) &&
                        (d_tests_matrix_parity<T, 2>(300)) &&
                        (d_tests_matrix_parity<T, 3>(300)) &&
                        (d_tests_matrix_parity<T, 4>(200)) &&
                        (d_tests_matrix_parity<T, 5>(100)),
                        "det, inverse and solve: cmat.h's, bit for bit");

        // sums and products; the bilinear dot, [1, i] . [1, i] = 1 + i i = 0
        const C                   i1(T(0), T(1));
        const C                   ve[2] = { C(T(1)), i1 };
        const la::vector<C, 2>    v(ve);
        const C                   ae[4] = { C(T(1)), i1, -i1, C(T(2)) };
        const la::matrix<C, 2, 2> a(ae);
        const la::matrix<C, 2, 2> sq = a.times(a);
        const la::vector<C, 2>    av = a.times(v);

        D_TESTS_EXPECT( (d_tests_same(v.dot(v), d_cplx_make(T(0), T(0)))) &&
                        (d_tests_same(sq(0, 0), d_cplx_make(T(2), T(0)))) &&
                        (d_tests_same(sq(0, 1), d_cplx_make(T(0), T(3)))) &&
                        (d_tests_same(av[0], d_cplx_make(T(0), T(0)))) &&
                        (d_tests_same(av[1], d_cplx_make(T(0), T(1)))) &&
                        (d_tests_same((v + v)[1], d_cplx_make(T(0), T(2)))) &&
                        (d_tests_same(a.trace(), d_cplx_make(T(3), T(0)))),
                        "products, sums and the trace; dot the bilinear sum, "
                        "the product's, as NumPy's dot");

        // tolerances are of T: equals, approx_equal, is_identity,
        // is_symmetric and is_invertible, given one or none
        const T                   eps = std::numeric_limits<T>::epsilon();
        const C                   near_e[4] = { C(T(1) + eps, T(0)), C(),
                                                C(), C(T(1), eps) };
        const la::matrix<C, 2, 2> near(near_e);
        const la::matrix<C, 2, 2> id = la::matrix<C, 2, 2>::identity();

        D_TESTS_EXPECT( (near.equals(id)) &&
                        (near.equals(id, T(2) * eps)) &&
                        (!near.equals(id, eps / T(2))) &&
                        (la::approx_equal(near, id, T(2) * eps)) &&
                        (near.is_identity()) &&
                        (!near.is_identity(eps / T(4))) &&
                        (near.is_symmetric(T(0))) &&
                        (!a.is_symmetric(T(1))) &&
                        (la::is_invertible(a)) &&
                        (la::is_invertible(a, T(0.5))) &&
                        (la::is_singular(a, T(1))) &&
                        (la::is_singular(la::matrix<C, 2, 2>())) &&
                        (!la::is_invertible(la::matrix<C, 2, 2>(), T(0))) &&
                        (v.equals(v, T(0))),
                        "tolerances of T, a magnitude |re| + |im|");

        // the inner product and the norms (S322, S354): vdot conjugates
        // its first operand; norm, length and norm_squared of the moduli,
        // the scaled norm of the parts; a unitary matrix's adjoint its
        // inverse
        const C                   we[2] = { C(T(3), T(4)), C(T(0), T(-12)) };
        const la::vector<C, 2>    w(we);
        const T                   parts[4] = { T(3), T(4), T(0), T(-12) };
        const T                   half = T(1) / T(2);
        const C                   ue[4] = { C(half, half), C(half, -half),
                                            C(half, -half), C(half, half) };
        const la::matrix<C, 2, 2> u(ue);
        const T                   aparts[8] = { T(1), T(0), T(0), T(1),
                                                T(0), T(-1), T(2), T(0) };
        const T                   big = std::numeric_limits<T>::max() / T(2);
        const T                   root3 = std::sqrt(T(3));
        const C                   he[2] = { C(big, big), C(big, T(0)) };
        const la::vector<C, 2>    huge(he);
        const la::vector<C, 2>    unit = la::normalize(w);
        const T                   re[2] = { T(1), T(2) };
        const T                   se[2] = { T(3), T(4) };
        const la::vector<T, 2>    r(re);
        const la::vector<T, 2>    s(se);
        const C                   ne[4] = { C(half + (T(32) * eps), half),
                                            C(half, -half),
                                            C(half, -half), C(half, half) };
        const la::matrix<C, 2, 2> nearly(ne);
        const T                   rot[4] = { T(0), T(-1), T(1), T(0) };
        const T                   shear[4] = { T(1), T(1), T(0), T(1) };

        D_TESTS_EXPECT( (d_tests_same(la::vdot(v, v),
                                      d_cplx_make(T(2), T(0)))) &&
                        (d_tests_same(la::vdot(w, v),
                                      d_cplx_make(T(-9), T(-4)))) &&
                        (d_tests_same(la::dot(v, v),
                                      d_cplx_make(T(0), T(0)))) &&
                        (d_tests_same_real(la::norm(w), T(13))) &&
                        (d_tests_same_real(la::length(w), T(13))) &&
                        (d_tests_same_real(la::norm_squared(w), T(169))) &&
                        (d_tests_same_real(
                             la::norm(w),
                             la::internal::linalg_kernel<T>::norm(parts,
                                                                  4))) &&
                        (d_tests_same_real(la::distance(w, w), T(0))) &&
                        (d_tests_same(unit[0],
                                      d_cplx_make(T(3) / T(13),
                                                  T(4) / T(13)))) &&
                        (d_tests_same_real(
                             la::frobenius_norm(a),
                             la::internal::linalg_kernel<T>::norm(aparts,
                                                                  8))) &&
                        (la::norm(huge) <= std::numeric_limits<T>::max()) &&
                        (la::norm(huge) >= (root3 * big) *
                                               (T(1) - (T(4) * eps))) &&
                        (la::norm(huge) <= (root3 * big) *
                                               (T(1) + (T(4) * eps))) &&
                        (la::is_unitary(u)) &&
                        (!la::is_orthogonal(u)) &&
                        (!la::is_unitary(a)) &&
                        (la::is_unitary(la::matrix<C, 2, 2>::identity(),
                                        T(0))) &&
                        (la::is_unitary(la::matrix<T, 2, 2>::identity())) &&
                        (la::vdot(r, s) == T(11)) &&
                        (la::vdot(r, s) == la::dot(r, s)) &&
                        (!la::is_unitary(nearly)) &&
                        (la::is_unitary(nearly, T(1) / T(1000))) &&
                        (la::is_unitary(la::matrix<T, 2, 2>(rot))) &&
                        (!la::is_unitary(la::matrix<T, 2, 2>(shear))) &&
                        (!la::is_unitary(la::matrix<T, 2, 2>(shear),
                                         T(1) / T(1000))),
                        "vdot conjugating its first operand, dot bilinear, "
                        "and of real vectors their dot; "
                        "norm, length and norm_squared of the moduli, the "
                        "scaled norm of the parts, past T's overflow where "
                        "the norm is not; normalize and distance; the "
                        "Frobenius norm; is_unitary within its tolerance, "
                        "given or is_identity's, of a real matrix "
                        "is_orthogonal (S322, S354)");

        return;
    }

    template<typename T>
    void
    d_tests_family()
    {
        m::complex<T> points[2000];

        d_tests_points(points, 2000);

        d_tests_construction<T>();
        d_tests_arithmetic<T>(points, 2000);
        d_tests_functions<T>(points, 2000);
        d_tests_numbers<T>();
        d_tests_matrices<T>();

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
namespace
{
    typedef m::complex<double> cd;

    // d_tests_table
    //   every function at two points, computed as a constant expression and
    // at run time: the same bits.
    struct d_tests_table
    {
        cd f[2][27];
    };

    constexpr d_tests_table
    d_tests_functions_at(
        const cd& _a,
        const cd& _b
    )
    {
        d_tests_table t{};
        const cd      at[2] = { _a, _b };

        for (int p = 0; p < 2; ++p)
        {
            const cd z = at[p];

            t.f[p][0]  = exp(z);
            t.f[p][1]  = log(z);
            t.f[p][2]  = sqrt(z);
            t.f[p][3]  = sin(z);
            t.f[p][4]  = cos(z);
            t.f[p][5]  = tan(z);
            t.f[p][6]  = asin(z);
            t.f[p][7]  = acos(z);
            t.f[p][8]  = atan(z);
            t.f[p][9]  = sinh(z);
            t.f[p][10] = cosh(z);
            t.f[p][11] = tanh(z);
            t.f[p][12] = asinh(z);
            t.f[p][13] = acosh(z);
            t.f[p][14] = atanh(z);
            t.f[p][15] = pow(z, at[1 - p]);
            t.f[p][16] = cd(abs(z), arg(z));
            t.f[p][17] = conj(z) * z / at[1 - p];
            t.f[p][18] = proj(z) - 2.0;
            t.f[p][19] = pow(z, 2.5);
            t.f[p][20] = pow(0.5, z);
            t.f[p][21] = -z + at[1 - p];
            t.f[p][22] = exp2(z);
            t.f[p][23] = log2(z);
            t.f[p][24] = log10(z);
            t.f[p][25] = log1p(z);
            t.f[p][26] = expm1(z);
        }

        return t;
    }

    constexpr d_tests_table g_constant =
        d_tests_functions_at(cd(0.75, -2.5), cd(-3.0, 0.125));

    static_assert( (cd(1, 2) * cd(3, 4) == cd(-5, 10)) &&
                   (abs(cd(3, 4)) == 5.0) &&
                   (sqrt(cd(0, 2)) == cd(1, 1)),
                   "the arithmetic and the functions as constant "
                   "expressions, from C++14");

    static_assert( (exp(m::complex<float>()) == 1.0f) &&
                   (exp(m::complex<long double>()) == 1.0L),
                   "float's and long double's as constant expressions");

    // a determinant as a constant expression: [[1, i], [-i, 2]], 1
    constexpr cd
    d_tests_constant_det()
    {
        const cd e[4] = { cd(1), cd(0, 1), cd(0, -1), cd(2) };

        return la::determinant(la::matrix<cd, 2, 2>(e));
    }

    static_assert(d_tests_constant_det() == 1.0,
                  "a complex determinant as a constant expression");

    void
    d_tests_constant_evaluation()
    {
        volatile double     a_re = 0.75;
        volatile double     b_re = -3.0;
        const d_tests_table run  = d_tests_functions_at(cd(a_re, -2.5),
                                                        cd(b_re, 0.125));
        bool                same = true;

        for (int p = 0; p < 2; ++p)
        {
            for (int k = 0; k < 27; ++k)
            {
                same = ( (same) &&
                         (d_tests_same(run.f[p][k], g_constant.f[p][k])) );
            }
        }

        D_TESTS_EXPECT(same, "every function as a constant expression: the "
                             "run time's bits");

        return;
    }
}  // namespace
#endif


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_constant_evaluation();
#endif

    std::printf("t_complex_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
