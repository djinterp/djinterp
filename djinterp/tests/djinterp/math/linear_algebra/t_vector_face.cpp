/*******************************************************************************
* djinterp [test]                                              t_vector_face.cpp
*
*   Tests for linear_algebra/vector.hpp, at every level from C++98. The
* compile-time half -- the traits, the families, and from C++14 (C++20 for an
* angle) the operations as constant expressions -- runs wherever the unit
* compiles. The runtime half checks what the face adds to the C core:
*
*     - the generic path agrees with the C families, bit for bit: a vector
*       of t_real, a user's type wrapping a double (the generic path),
*       against the same vector of double (the C core), every operation;
*     - every member and free function of float, double and long double
*       vectors against its formula, the roots against the C library's;
*     - an integer vector (the generic path, as the subframework always
*       computed it: roots through double, truncated);
*     - the constructors at every level -- one to four components, an array
*       -- and from C++11 std::array and five components and more;
*     - angles: parallel vectors at 0, not NaN; perpendicular ones at pi/2.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_vector_face.cpp -o t_vector -lm
*
*
* path:      /tests/djinterp/math/linear_algebra/t_vector_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../../inc/djinterp/math/linear_algebra/vector.hpp"  // tested
// std
#include <cmath>    // std::sqrt, std::acos
#include <cstdio>   // std::printf


namespace
{
    using djinterp::math::linalg::vector;

    // t_real
    //   a user's numeric type, wrapping a double: not arithmetic, so the
    // generic path serves it.
    struct t_real
    {
        double v;

        t_real() : v(0.0) {}
        t_real(double _v) : v(_v) {}

        operator double() const { return v; }

        t_real& operator+=(const t_real& _o) { v += _o.v; return *this; }
        t_real& operator*=(const t_real& _o) { v *= _o.v; return *this; }
    };

    t_real operator+(t_real _a, t_real _b) { return t_real(_a.v + _b.v); }
    t_real operator-(t_real _a, t_real _b) { return t_real(_a.v - _b.v); }
    t_real operator*(t_real _a, t_real _b) { return t_real(_a.v * _b.v); }
    t_real operator/(t_real _a, t_real _b) { return t_real(_a.v / _b.v); }
    t_real operator-(t_real _a) { return t_real(-_a.v); }
    bool operator<(t_real _a, t_real _b) { return _a.v < _b.v; }
    bool operator>(t_real _a, t_real _b) { return _a.v > _b.v; }
    bool operator==(t_real _a, t_real _b) { return _a.v == _b.v; }

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

    // the same once each is stored: on x87 a value in a register can carry
    // excess precision its twin does not
    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    unsigned long g_state = 2463534242u;

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    // d_tests_generic_parity
    //   a t_real vector (the generic path) against a double one (the C
    // core), every operation, bit for bit.
    void
    d_tests_generic_parity()
    {
        for (int trial = 0; trial < 20000; ++trial)
        {
            double a_d[4];
            double b_d[4];
            t_real a_r[4];
            t_real b_r[4];

            for (int i = 0; i < 4; ++i)
            {
                a_d[i] = d_tests_random();
                b_d[i] = d_tests_random();
                a_r[i] = t_real(a_d[i]);
                b_r[i] = t_real(b_d[i]);
            }

            const vector<double, 4> a(a_d);
            const vector<double, 4> b(b_d);
            const vector<t_real, 4> ar(a_r);
            const vector<t_real, 4> br(b_r);
            const double            s = d_tests_random();

            const vector<double, 4> sums[] = { a.plus(b), a.minus(b),
                                               a.scaled(s), a.hadamard(b),
                                               a.lerp(b, s), a.normalized(),
                                               a.projected_onto(b),
                                               a.rejected_from(b),
                                               a.negated() };
            const vector<t_real, 4> sums_r[] = { ar.plus(br), ar.minus(br),
                                                 ar.scaled(s),
                                                 ar.hadamard(br),
                                                 ar.lerp(br, s),
                                                 ar.normalized(),
                                                 ar.projected_onto(br),
                                                 ar.rejected_from(br),
                                                 ar.negated() };

            for (int k = 0; k < 9; ++k)
            {
                bool same = true;

                for (int i = 0; i < 4; ++i)
                {
                    same = same && d_tests_same(sums[k][i], sums_r[k][i].v);
                }

                D_TESTS_EXPECT(same, "a vector operation, generic and C");
            }

            D_TESTS_EXPECT(d_tests_same(a.dot(b), ar.dot(br).v),
                           "dot, generic and C");
            D_TESTS_EXPECT(d_tests_same(a.norm(), ar.norm().v),
                           "norm, generic and C");
            D_TESTS_EXPECT(d_tests_same(a.distance(b), ar.distance(br).v),
                           "distance, generic and C");
            D_TESTS_EXPECT(d_tests_same(a.cos_angle(b), ar.cos_angle(br).v),
                           "cos_angle, generic and C");
            D_TESTS_EXPECT(d_tests_same(a.angle_to(b), ar.angle_to(br).v),
                           "angle_to, generic and C");
            D_TESTS_EXPECT( (ar == ar) &&
                            (a == a),
                            "a vector equals itself, generic and C");
            D_TESTS_EXPECT( (d_tests_same(a.sum(), ar.sum().v)) &&
                            (d_tests_same(a.product(), ar.product().v)) &&
                            (d_tests_same(a.min_coeff(), ar.min_coeff().v)) &&
                            (d_tests_same(a.max_coeff(), ar.max_coeff().v)),
                            "reductions, generic and C");
        }

        return;
    }

    // d_tests_family
    //   one C family against the formulas and the C library.
    template<typename Type>
    void
    d_tests_family()
    {
        const vector<Type, 3> a(static_cast<Type>(1),
                                static_cast<Type>(2),
                                static_cast<Type>(2));
        const vector<Type, 3> b(3, -1, 0.5);
        const vector<Type, 3> zero;

        D_TESTS_EXPECT(a.norm() == static_cast<Type>(3), "norm of (1, 2, 2)");
        D_TESTS_EXPECT(d_tests_same(b.norm(),
                                    static_cast<Type>(std::sqrt(
                                        static_cast<Type>(10.25)))),
                       "norm is the library root");
        D_TESTS_EXPECT( (a.cross(b)[0] == static_cast<Type>(1 + 2)) &&
                        (a.cross(b)[1] == static_cast<Type>(6 - 0.5)) &&
                        (a.cross(b)[2] == static_cast<Type>(-1 - 6)),
                        "cross");
        D_TESTS_EXPECT(a.dot(b) == static_cast<Type>(3 - 2 + 1), "dot");
        D_TESTS_EXPECT(zero.normalized() == zero,
                       "the zero vector normalizes to zero");
        D_TESTS_EXPECT(a.normalized().is_unit(static_cast<Type>(1e-6)),
                       "a normalized vector is a unit");
        D_TESTS_EXPECT(a.angle_to(a.scaled(static_cast<Type>(7))) ==
                       static_cast<Type>(0),
                       "parallel vectors are at angle 0");
        D_TESTS_EXPECT(d_tests_same(
                           vector<Type, 2>(1, 0).angle_to(
                               vector<Type, 2>(0, 1)),
                           static_cast<Type>(std::acos(static_cast<Type>(0)))),
                       "perpendicular vectors are at the library's pi/2");
        D_TESTS_EXPECT(a.cos_angle(zero) == static_cast<Type>(0),
                       "the zero vector's cosine is 0");
        D_TESTS_EXPECT( (a + b == a.plus(b)) &&
                        (a - b == a.minus(b)) &&
                        (-a == a.negated()) &&
                        (a * 2 == a.scaled(static_cast<Type>(2))) &&
                        (2 * a == a.scaled(static_cast<Type>(2))) &&
                        (a / 2 == a.divided(static_cast<Type>(2))) &&
                        (a != b),
                        "the operators");
        // (scalars compared once stored: x87 keeps excess precision)
        D_TESTS_EXPECT( (d_tests_same(dot(a, b), a.dot(b))) &&
                        (d_tests_same(norm(a), a.norm())) &&
                        (d_tests_same(length(a), a.norm())) &&
                        (d_tests_same(distance(a, b), a.distance(b))) &&
                        (cross(a, b) == a.cross(b)) &&
                        (lerp(a, b, static_cast<Type>(0.25)) ==
                         a.lerp(b, static_cast<Type>(0.25))) &&
                        (approx_equal(a, a)),
                        "the procedural spellings");
        D_TESTS_EXPECT( (vector<Type, 3>::template basis<1>()[1] ==
                         static_cast<Type>(1)) &&
                        (vector<Type, 3>::filled(static_cast<Type>(4))
                             .sum() == static_cast<Type>(12)),
                        "the factories");
        D_TESTS_EXPECT( (a.with(0, static_cast<Type>(9)).x() ==
                         static_cast<Type>(9)) &&
                        (a.y() == static_cast<Type>(2)) &&
                        (a.z() == static_cast<Type>(2)),
                        "access");

        return;
    }

    struct t_square
    {
        double operator()(double _c) const { return _c * _c; }
    };

    struct t_add
    {
        double operator()(double _acc, double _c) const { return _acc + _c; }
    };

    void
    d_tests_generic_integers()
    {
        const vector<int, 2> v(3, 4);

        // the subframework's roots for an integer: through double, truncated
        D_TESTS_EXPECT(v.norm() == 5, "an int vector's norm");
        D_TESTS_EXPECT( (v.normalized()[0] == 0) &&
                        (v.normalized()[1] == 0),
                        "an int vector normalizes by integer division");
        D_TESTS_EXPECT((vector<int, 2>(1, 1).distance(vector<int, 2>(4, 5)) ==
                        5),
                       "an int distance");

        // the functional bridge
        const vector<double, 3> d(1, 2, 3);

        D_TESTS_EXPECT( (d.map(t_square()).sum() == 14.0) &&
                        (d.reduce(0.0, t_add()) == 6.0),
                        "map and reduce");

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    void
    d_tests_cpp11()
    {
        const std::array<double, 3> parts = { { 1.0, 2.0, 2.0 } };
        const vector<double, 5>     five(1, 2, 3, 4, 5);

        D_TESTS_EXPECT( (vector<double, 3>::from_array(parts).norm() == 3.0) &&
                        (vector<double, 3>(parts).norm() == 3.0),
                        "std::array interop");
        D_TESTS_EXPECT(five.sum() == 15.0, "five components");

        return;
    }
#endif
}  // namespace

// the traits and the families, at every level
D_STATIC_ASSERT((djinterp::math::linalg::is_vector<vector<double, 3> >::value),
                "is_vector");
D_STATIC_ASSERT((!djinterp::math::linalg::is_vector<double>::value),
                "a double is not a vector");
D_STATIC_ASSERT((djinterp::math::linalg::internal::linalg_family_of<
                     long double>::value == 1),
                "long double reaches the C core");
D_STATIC_ASSERT((djinterp::math::linalg::internal::linalg_family_of<
                     int>::value == 0),
                "int takes the generic path");
D_STATIC_ASSERT((sizeof(vector<float, 3>) == 3 * sizeof(float)),
                "a vector is its components and nothing more");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
// the operations as constant expressions, roots included, from C++14
static_assert(vector<double, 3>(1, 2, 2).norm() == 3.0, "constexpr norm");
static_assert(vector<double, 3>(1, 0, 0).cross(vector<double, 3>(0, 1, 0))[2]
                  == 1.0,
              "constexpr cross");
static_assert(vector<float, 2>(0, 2).normalized()[1] == 1.0f,
              "constexpr normalized");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// and an angle from C++20
static_assert(vector<double, 2>(1, 0).angle_to(vector<double, 2>(0, 1)) >
                  1.5707963267948,
              "constexpr angle");
#endif


int
main()
{
    d_tests_generic_parity();
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_generic_integers();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_cpp11();
#endif

    std::printf("t_vector_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
