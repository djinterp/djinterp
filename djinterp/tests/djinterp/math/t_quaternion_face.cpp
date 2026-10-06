/*******************************************************************************
* djinterp [test]                                          t_quaternion_face.cpp
*
*   Tests for both quaternion headers -- math/quaternion.hpp and
* math/linear_algebra/quaternion.hpp -- at every level from C++98: from an
* axis and angle, each the C library's sine and cosine in its own formula,
* bit for bit; the Hamilton product as the headers wrote it; the norm the
* correctly rounded root; rotation equal to the matrix's product, q and -q
* the same rotation, q q* the identity; slerp's ends and midpoint; the
* generic path (t_real, a user's type wrapping a double) against the C
* core, bit for bit; and the algebra as constant expressions from C++14,
* from_axis_angle from C++20.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/t_quaternion_face.cpp -o t_quaternion -lm
*
*
* path:      /tests/djinterp/math/t_quaternion_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.05
*******************************************************************************/
#include "../../../inc/djinterp/math/quaternion.hpp"  // tested
// djinterp
#include "../../../inc/djinterp/math/linear_algebra/quaternion.hpp"  // tested
// std
#include <cmath>    // std::sin, std::cos, std::sqrt
#include <cstdio>   // std::printf


namespace
{
    using djinterp::math::linalg::matrix;
    using djinterp::math::linalg::vector;
    typedef djinterp::math::quaternion<double> mq;

    // t_real
    //   a user's numeric type, wrapping a double: the generic path.
    struct t_real
    {
        double v;

        t_real() : v(0.0) {}
        t_real(double _v) : v(_v) {}

        operator double() const { return v; }

        t_real& operator+=(const t_real& _o) { v += _o.v; return *this; }
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

    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    template<typename Float>
    bool
    d_tests_near(Float _a, Float _b, Float _tol)
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    unsigned long g_state = 987654321u;

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<typename Type>
    void
    d_tests_family(Type _tol)
    {
        typedef djinterp::math::linalg::quaternion<Type> lq;
        typedef djinterp::math::quaternion<Type>         q;

        for (int trial = 0; trial < 3000; ++trial)
        {
            const vector<Type, 3> axis(d_tests_random(),
                                       d_tests_random(),
                                       d_tests_random());
            const Type            angle = static_cast<Type>(d_tests_random());
            const Type            half  = angle / static_cast<Type>(2);
            const Type            sine  = std::sin(half);
            const Type            cosine = std::cos(half);

            // from an axis and angle: the library's sine and cosine, in each
            // header's own formula
            const lq a = djinterp::math::linalg::from_axis_angle(axis, angle);
            // (the length stored: on an x87 a returned double may carry
            // excess precision, which only an assignment or a cast removes)
            volatile Type length = axis.length();
            const Type    s      = sine / static_cast<Type>(length);

            D_TESTS_EXPECT( (d_tests_same(a.m_x, axis[0] * s)) &&
                            (d_tests_same(a.m_z, axis[2] * s)) &&
                            (d_tests_same(a.m_w, cosine)),
                            "linalg::from_axis_angle");

            const q b = djinterp::math::from_axis_angle(axis, angle);
            const vector<Type, 3> unit = axis.normalized();

            D_TESTS_EXPECT( (d_tests_same(b.x, unit[0] * sine)) &&
                            (d_tests_same(b.y, unit[1] * sine)) &&
                            (d_tests_same(b.w, cosine)),
                            "math::from_axis_angle");

            // the norm is the correctly rounded root, the product as written
            const lq c(a.m_y, a.m_w, a.m_x, a.m_z);
            const lq ac = a * c;

            // (each step stored: an x87 rounds a whole expression once)
            volatile Type xx = a.m_x * a.m_x;
            volatile Type yy = a.m_y * a.m_y;
            volatile Type zz = a.m_z * a.m_z;
            volatile Type ww = a.m_w * a.m_w;
            volatile Type s1 = xx + yy;
            volatile Type s2 = s1 + zz;
            volatile Type s3 = s2 + ww;
            volatile Type p1 = a.m_w * c.m_w;
            volatile Type p2 = a.m_x * c.m_x;
            volatile Type p3 = a.m_y * c.m_y;
            volatile Type p4 = a.m_z * c.m_z;
            volatile Type d1 = p1 - p2;
            volatile Type d2 = d1 - p3;
            volatile Type d3 = d2 - p4;

            D_TESTS_EXPECT(d_tests_same(djinterp::math::linalg::norm(a),
                                        static_cast<Type>(std::sqrt(
                                            static_cast<Type>(s3)))),
                           "the norm is the library root");
            D_TESTS_EXPECT(d_tests_same(ac.m_w, static_cast<Type>(d3)),
                           "the Hamilton product as written");

            // rotation: the matrix's product, q and -q, q q*
            const vector<Type, 3> v(d_tests_random(), d_tests_random(),
                                    d_tests_random());
            const vector<Type, 3> r  = djinterp::math::linalg::rotate(a, v);
            const vector<Type, 3> rm =
                djinterp::math::linalg::to_matrix3(a).times(v);
            const lq              na(-a.m_x, -a.m_y, -a.m_z, -a.m_w);

            D_TESTS_EXPECT(r.equals(rm, static_cast<Type>(_tol * 64)),
                           "rotation is the matrix's product");
            D_TESTS_EXPECT(djinterp::math::linalg::rotate(na, v) == r,
                           "q and -q are the same rotation");
            D_TESTS_EXPECT( (d_tests_near((a * djinterp::math::linalg::
                                               conjugate(a)).m_w,
                                          static_cast<Type>(1), _tol)) &&
                            (djinterp::math::rotate(b, v).equals(
                                 djinterp::math::to_matrix(b).times(v),
                                 static_cast<Type>(_tol * 64))),
                            "q q* = 1; math::rotate is its matrix's");
            D_TESTS_EXPECT( (djinterp::math::to_matrix4(b)(3, 3) ==
                             static_cast<Type>(1)) &&
                            (djinterp::math::to_matrix4(b)(0, 1) ==
                             djinterp::math::to_matrix(b)(0, 1)) &&
                            (djinterp::math::dot(b, b) ==
                             djinterp::math::norm(b) *
                                 djinterp::math::norm(b) ||
                             d_tests_near(djinterp::math::dot(b, b),
                                          static_cast<Type>(1), _tol)),
                            "math::to_matrix4, dot and norm");
        }

        // slerp: an eighth turn halfway to a quarter turn
        const vector<Type, 3> z(0, 0, 1);
        const lq quarter = djinterp::math::linalg::from_axis_angle(
            z, static_cast<Type>(1.57079632679489661923L));
        const lq eighth  = djinterp::math::linalg::slerp(
            lq::identity(), quarter, static_cast<Type>(0.5));
        const vector<Type, 3> image = djinterp::math::linalg::rotate(
            eighth, vector<Type, 3>(1, 0, 0));

        D_TESTS_EXPECT( (d_tests_near(image[0],
                                      static_cast<Type>(0.7071067811865476L),
                                      _tol)) &&
                        (d_tests_near(image[1],
                                      static_cast<Type>(0.7071067811865476L),
                                      _tol)),
                        "slerp halfway to a quarter turn");
        D_TESTS_EXPECT(djinterp::math::linalg::normalize(
                           djinterp::math::linalg::slerp(
                               lq::identity(), quarter, static_cast<Type>(1)))
                           .m_w == quarter.m_w ||
                       d_tests_near(djinterp::math::linalg::slerp(
                                        lq::identity(), quarter,
                                        static_cast<Type>(1)).m_w,
                                    quarter.m_w, _tol),
                       "slerp at 1");
        D_TESTS_EXPECT(djinterp::math::slerp(q(), q(), static_cast<Type>(0.3))
                           .w == static_cast<Type>(1),
                       "math::slerp between equal rotations");

        return;
    }

    void
    d_tests_generic_parity()
    {
        typedef djinterp::math::linalg::quaternion<double> lq;
        typedef djinterp::math::linalg::quaternion<t_real> lr;

        for (int trial = 0; trial < 3000; ++trial)
        {
            const double e[8] = { d_tests_random(), d_tests_random(),
                                  d_tests_random(), d_tests_random(),
                                  d_tests_random(), d_tests_random(),
                                  d_tests_random(), d_tests_random() };
            const lq a(e[0], e[1], e[2], e[3]);
            const lq b(e[4], e[5], e[6], e[7]);
            const lr ar(e[0], e[1], e[2], e[3]);
            const lr br(e[4], e[5], e[6], e[7]);
            const lq p  = a * b;
            const lr pr = ar * br;

            D_TESTS_EXPECT( (d_tests_same(p.m_x, pr.m_x.v)) &&
                            (d_tests_same(p.m_y, pr.m_y.v)) &&
                            (d_tests_same(p.m_z, pr.m_z.v)) &&
                            (d_tests_same(p.m_w, pr.m_w.v)),
                            "the product, generic and C");
            D_TESTS_EXPECT(d_tests_same(djinterp::math::linalg::norm(a),
                                        djinterp::math::linalg::norm(ar).v),
                           "the norm, generic and C");

            const lq na = djinterp::math::linalg::normalize(a);
            const lr nr = djinterp::math::linalg::normalize(ar);
            const vector<double, 3> v(e[4], e[5], e[6]);
            const t_real            vr_e[3] = { e[4], e[5], e[6] };
            const vector<t_real, 3> vr(vr_e);
            const vector<double, 3> r  = djinterp::math::linalg::rotate(na, v);
            const vector<t_real, 3> rr = djinterp::math::linalg::rotate(nr, vr);
            const matrix<double, 3, 3> m  =
                djinterp::math::linalg::to_matrix3(na);
            const matrix<t_real, 3, 3> mr =
                djinterp::math::linalg::to_matrix3(nr);
            const lq s  = djinterp::math::linalg::slerp(na,
                              djinterp::math::linalg::normalize(b), 0.25);
            const lr sr = djinterp::math::linalg::slerp(nr,
                              djinterp::math::linalg::normalize(br),
                              t_real(0.25));

            D_TESTS_EXPECT( (d_tests_same(na.m_w, nr.m_w.v)) &&
                            (d_tests_same(r[0], rr[0].v)) &&
                            (d_tests_same(r[2], rr[2].v)) &&
                            (d_tests_same(m(0, 1), mr(0, 1).v)) &&
                            (d_tests_same(m(2, 2), mr(2, 2).v)) &&
                            (d_tests_same(s.m_x, sr.m_x.v)) &&
                            (d_tests_same(s.m_w, sr.m_w.v)),
                            "normalize, rotate, to_matrix3, slerp: "
                            "generic and C");
        }

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(djinterp::math::linalg::norm(
                  djinterp::math::linalg::quaternion<double>(0, 0, 0, 2)) ==
                  2.0,
              "a constexpr norm");
static_assert(djinterp::math::norm(djinterp::math::quaternion<double>(
                  0.0, 0.0, 3.0, 4.0)) == 5.0,
              "math's constexpr norm");
static_assert((djinterp::math::linalg::quaternion<double>::identity() *
               djinterp::math::linalg::quaternion<double>::identity())
                  .m_w == 1.0,
              "a constexpr product");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert(djinterp::math::linalg::from_axis_angle(
                  vector<double, 3>(0, 0, 1), 0.0).m_w == 1.0,
              "a constexpr rotation from an axis and angle");
#endif


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);
    d_tests_generic_parity();

    (void)sizeof(mq);

    std::printf("t_quaternion_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
