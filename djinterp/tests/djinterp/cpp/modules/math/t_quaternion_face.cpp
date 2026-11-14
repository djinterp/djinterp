/*******************************************************************************
* djinterp [cpp/math]                                      t_quaternion_face.cpp
*
*   Tests for both quaternion headers -- math/quaternion.hpp and
* math/linear_algebra/quaternion.hpp -- at every level from C++98: from an
* axis and angle, each the C library's sine and cosine in its own formula,
* bit for bit; the Hamilton product, the norm and the rotation within a
* step of the correctly rounded value, taken in a wider type; the _fast
* forms the C core's former formulas, bit for bit; rotation equal to the
* matrix's product, q and -q the same rotation, q q^-1 the identity, the
* quotients undoing the product; Shepperd's method inverting the matrix, two
* vectors' quaternion turning one into the other; slerp's ends and midpoint,
* nlerp's ends; exp and log, the square root squared, a power as products;
* each face's function the C core's, bit for bit, in the form the face takes
* at run time at its level; the generic path (t_real, a user's type wrapping
* a double) against the C core: the _fast forms and slerp, exp, log, sqrt
* and pow bit for bit, through double, and the accurate algebra, the matrix
* and Shepperd's method of a q off unit length, and two vectors' quaternion,
* of opposed vectors too, within a few steps; and everything but
* from_axis_angle, slerp_fast and normalize_fast as constant expressions
* from C++14, those two from C++20.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/t_quaternion_face.cpp                \
*         -o t_quaternion -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_quaternion_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/quaternion.hpp"  // tested
// djinterp
#include "../../../../../inc/djinterp/cpp/modules/math/linear_algebra/quaternion.hpp"  // tested
// std
#include <cmath>    // std::sin, std::cos, std::sqrt, std::fabs
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

    // d_tests_steps_ok
    //   whether _got is within _steps of the family's values of _want, a
    // wider type's value: the correctly rounded value's neighbourhood.
    template<typename Float>
    bool
    d_tests_steps_ok(Float _got, long double _want, int _steps)
    {
        volatile Float value = static_cast<Float>(_want);
        Float          low   = value;
        Float          high  = value;

        for (int i = 0; i < _steps; ++i)
        {
            low  = d_math_next_down(low);
            high = d_math_next_up(high);
        }

        return ( (!(_got < low)) &&
                 (!(_got > high)) );
    }

    template<typename Q>
    bool
    d_tests_quat_near(const Q& _a, const Q& _b, double _tol)
    {
        return ( (d_tests_near<double>(_a.m_x, _b.m_x, _tol)) &&
                 (d_tests_near<double>(_a.m_y, _b.m_y, _tol)) &&
                 (d_tests_near<double>(_a.m_z, _b.m_z, _tol)) &&
                 (d_tests_near<double>(_a.m_w, _b.m_w, _tol)) );
    }

    template<typename Q>
    bool
    d_tests_quat_same(const Q& _a, const Q& _b)
    {
        return ( (d_tests_same(_a.m_x, _b.m_x)) &&
                 (d_tests_same(_a.m_y, _b.m_y)) &&
                 (d_tests_same(_a.m_z, _b.m_z)) &&
                 (d_tests_same(_a.m_w, _b.m_w)) );
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
        namespace la = djinterp::math::linalg;
        namespace ma = djinterp::math;

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
            const lq a = la::from_axis_angle(axis, angle);
            // (the length stored: on an x87 a returned double may carry
            // excess precision, which only an assignment or a cast removes)
            volatile Type length = axis.length();
            const Type    s      = sine / static_cast<Type>(length);

            D_TESTS_EXPECT( (d_tests_same(a.m_x, axis[0] * s)) &&
                            (d_tests_same(a.m_z, axis[2] * s)) &&
                            (d_tests_same(a.m_w, cosine)),
                            "linalg::from_axis_angle");

            const q b = ma::from_axis_angle(axis, angle);
            const vector<Type, 3> unit = axis.normalized();

            D_TESTS_EXPECT( (d_tests_same(b.x, unit[0] * sine)) &&
                            (d_tests_same(b.y, unit[1] * sine)) &&
                            (d_tests_same(b.w, cosine)),
                            "math::from_axis_angle");

            // the norm, the product and the fast product: the C core's,
            // which t_quat.c holds to its references
            const lq c(a.m_y, a.m_w, a.m_x, a.m_z);
            const lq ac = a * c;
            Type ea[4] = { a.m_x, a.m_y, a.m_z, a.m_w };
            Type ec[4] = { c.m_x, c.m_y, c.m_z, c.m_w };
            Type ep[4] = { Type(), Type(), Type(), Type() };
            Type ef[4] = { Type(), Type(), Type(), Type() };

            d_quat_multiply(ep, ea, ec);
            d_quat_multiply_fast(ef, ea, ec);
            D_TESTS_EXPECT( (d_tests_same(la::norm(a), d_quat_norm(ea))) &&
                            (d_tests_same(ma::norm(b), la::norm(lq(b.x, b.y,
                                                                   b.z,
                                                                   b.w)))),
                           "the norm is the C core's");
            D_TESTS_EXPECT( (d_tests_same(ac.m_w, ep[3])) &&
                            (d_tests_same(ac.m_y, ep[1])),
                           "the Hamilton product is the C core's");
            D_TESTS_EXPECT( (d_tests_same(la::multiply_fast(a, c).m_w,
                                          ef[3])) &&
                            (d_tests_same(la::multiply_fast(a, c).m_x,
                                          ef[0])),
                            "multiply_fast is the C core's former formula");

            // rotation: the matrix's product, q and -q, q q^-1; the fast
            // forms the C core's
            const vector<Type, 3> v(d_tests_random(), d_tests_random(),
                                    d_tests_random());
            const vector<Type, 3> r  = la::rotate(a, v);
            const vector<Type, 3> rm = la::to_matrix3(a).times(v);
            const lq              na(-a.m_x, -a.m_y, -a.m_z, -a.m_w);
            const lq              one;

            D_TESTS_EXPECT(r.equals(rm, static_cast<Type>(_tol * 64)),
                           "rotation is the matrix's product");
            D_TESTS_EXPECT(la::rotate(na, v) == r,
                           "q and -q are the same rotation");
            D_TESTS_EXPECT(d_tests_quat_near(a * la::inverse(a), one,
                                             static_cast<double>(_tol)),
                           "q q^-1 = 1");
            D_TESTS_EXPECT( (d_tests_quat_near((a * c) / c, a,
                                               static_cast<double>(_tol * 8)))
                            &&
                            (d_tests_quat_near(la::left_divide(a, a * c), c,
                                               static_cast<double>(_tol * 8))),
                            "(a c) / c = a, a \\ (a c) = c");
            D_TESTS_EXPECT( (ma::rotate(b, v).equals(
                                 ma::to_matrix(b).times(v),
                                 static_cast<Type>(_tol * 64))) &&
                            (d_tests_near(ma::norm(b * ma::inverse(b)),
                                          static_cast<Type>(1), _tol)),
                            "math::rotate is its matrix's; q q^-1");
            D_TESTS_EXPECT( (ma::to_matrix4(b)(3, 3) ==
                             static_cast<Type>(1)) &&
                            (ma::to_matrix4(b)(0, 1) ==
                             ma::to_matrix(b)(0, 1)) &&
                            (d_tests_near(ma::dot(b, b),
                                          static_cast<Type>(1), _tol)),
                            "math::to_matrix4, dot and norm");
            {
                Type vf[3] = { Type(), Type(), Type() };
                Type mf[9] = { Type(), Type(), Type(), Type(), Type(),
                               Type(), Type(), Type(), Type() };
                Type nf[4] = { Type(), Type(), Type(), Type() };

                d_quat_rotate_fast(vf, ea, v.data());
                d_quat_to_matrix3_fast(mf, ea);
                d_quat_normalize_fast(nf, ec);
                D_TESTS_EXPECT( (d_tests_same(la::rotate_fast(a, v)[0],
                                              vf[0])) &&
                                (d_tests_same(ma::rotate_fast(b, v)[2],
                                              ma::rotate_fast(b, v)[2])) &&
                                (d_tests_same(la::to_matrix3_fast(a)(1, 2),
                                              mf[5])) &&
                                (d_tests_same(la::to_matrix4_fast(a)(2, 0),
                                              mf[6])) &&
                                (d_tests_same(la::normalize_fast(c).m_y,
                                              nf[1])),
                                "the _fast forms are the C core's");
            }

            // Shepperd's method inverts the matrix; two vectors' quaternion
            const lq unit_a = la::normalize(a * c);
            const lq back   = la::from_matrix(la::to_matrix3(unit_a));
            const lq back4  = la::from_matrix(la::to_matrix4(unit_a));
            const lq signed_a = (unit_a.m_w < static_cast<Type>(0)) ?
                                    lq(-unit_a.m_x, -unit_a.m_y,
                                       -unit_a.m_z, -unit_a.m_w) :
                                    unit_a;

            D_TESTS_EXPECT( (d_tests_quat_near(back, signed_a,
                                               static_cast<double>(_tol * 8)))
                            &&
                            (d_tests_quat_same(back, back4)),
                            "from_matrix inverts to_matrix3");

            const lq turn = la::from_two_vectors(axis, v);
            const vector<Type, 3> image = la::rotate(turn, axis);

            D_TESTS_EXPECT(image.normalized().equals(
                               v.normalized(), static_cast<Type>(_tol * 64)),
                           "from_two_vectors turns a into b");
            D_TESTS_EXPECT( (ma::from_two_vectors(axis, v).x == turn.m_x) &&
                            (ma::from_two_vectors(axis, v).w == turn.m_w),
                            "the two faces' two vectors agree");

            // exp and log; the root squared; a power as products
            const lq e = la::exp(la::log(a * c));

            D_TESTS_EXPECT(d_tests_quat_near(e, a * c,
                                             static_cast<double>(_tol * 8)),
                           "exp(log q) = q");
            D_TESTS_EXPECT( (d_tests_quat_near(la::sqrt(ac) * la::sqrt(ac),
                                               ac,
                                               static_cast<double>(_tol * 8)))
                            &&
                            (d_tests_quat_near(la::pow(ac,
                                                       static_cast<Type>(2)),
                                               ac * ac,
                                               static_cast<double>(_tol * 32))),
                            "sqrt(q)^2 = q, q^2 = q q");
            D_TESTS_EXPECT( (d_tests_near(ma::exp(ma::log(b)).y, b.y,
                                          static_cast<Type>(_tol * 8))) &&
                            (d_tests_near(ma::sqrt(b).w * ma::sqrt(b).w -
                                              ma::sqrt(b).x * ma::sqrt(b).x -
                                              ma::sqrt(b).y * ma::sqrt(b).y -
                                              ma::sqrt(b).z * ma::sqrt(b).z,
                                          b.w, static_cast<Type>(_tol * 8)))
                            &&
                            (d_tests_near(ma::pow(b, static_cast<Type>(3)).w,
                                          (b * b * b).w,
                                          static_cast<Type>(_tol * 8))),
                            "math's exp, log, sqrt and pow");

            // slerp's ends exact; nlerp's ends normalized; slerp_fast near
            const lq ua = la::normalize(a);
            const lq uc = la::normalize(c);

            D_TESTS_EXPECT( (d_tests_quat_same(
                                 la::slerp(ua, uc, static_cast<Type>(0)),
                                 ua)) &&
                            (d_tests_quat_near(
                                 la::slerp_fast(ua, uc,
                                                static_cast<Type>(0.375)),
                                 la::slerp(ua, uc, static_cast<Type>(0.375)),
                                 static_cast<double>(_tol * 8))) &&
                            (d_tests_quat_same(
                                 la::nlerp(ua, uc, static_cast<Type>(0)),
                                 la::normalize(ua))),
                            "slerp's and nlerp's ends; slerp_fast");
        }

        // slerp: an eighth turn halfway to a quarter turn
        const vector<Type, 3> z(0, 0, 1);
        const lq quarter = la::from_axis_angle(
            z, static_cast<Type>(1.57079632679489661923L));
        const lq eighth  = la::slerp(lq::identity(), quarter,
                                     static_cast<Type>(0.5));
        const vector<Type, 3> image = la::rotate(eighth,
                                                 vector<Type, 3>(1, 0, 0));

        D_TESTS_EXPECT( (d_tests_near(image[0],
                                      static_cast<Type>(0.7071067811865476L),
                                      _tol)) &&
                        (d_tests_near(image[1],
                                      static_cast<Type>(0.7071067811865476L),
                                      _tol)),
                        "slerp halfway to a quarter turn");
        D_TESTS_EXPECT(d_tests_quat_same(la::slerp(lq::identity(), quarter,
                                                   static_cast<Type>(1)),
                                         quarter),
                       "slerp at 1");
        D_TESTS_EXPECT(ma::slerp(q(), q(), static_cast<Type>(0.3)).w ==
                           static_cast<Type>(1),
                       "math::slerp between equal rotations");

        // log(-1) = pi i
        D_TESTS_EXPECT(la::log(lq(0, 0, 0, -1)).m_x ==
                           static_cast<Type>(
                               3.14159265358979323846264338327950288L),
                       "log(-1) = pi i");

        return;
    }

    // d_tests_lq_is, d_tests_mq_is
    //   whether a face's quaternion holds the four parts _p, bit for bit.
    template<typename Type>
    bool
    d_tests_lq_is(const djinterp::math::linalg::quaternion<Type>& _q,
                  const Type*                                      _p)
    {
        return ( (d_tests_same(_q.m_x, _p[0])) &&
                 (d_tests_same(_q.m_y, _p[1])) &&
                 (d_tests_same(_q.m_z, _p[2])) &&
                 (d_tests_same(_q.m_w, _p[3])) );
    }

    template<typename Type>
    bool
    d_tests_mq_is(const djinterp::math::quaternion<Type>& _q,
                  const Type*                             _p)
    {
        return ( (d_tests_same(_q.x, _p[0])) &&
                 (d_tests_same(_q.y, _p[1])) &&
                 (d_tests_same(_q.z, _p[2])) &&
                 (d_tests_same(_q.w, _p[3])) );
    }

    // d_tests_matrix_is
    //   whether an N x N matrix holds the N^2 entries _p by rows, bit for
    // bit.
    template<typename Type, std::size_t N>
    bool
    d_tests_matrix_is(const matrix<Type, N, N>& _m,
                      const Type*               _p)
    {
        bool same = true;

        for (std::size_t i = 0; i < N; ++i)
        {
            for (std::size_t j = 0; j < N; ++j)
            {
                same = same && d_tests_same(_m(i, j), _p[(i * N) + j]);
            }
        }

        return same;
    }

    // d_tests_forwards
    //   each face's function the C core's, bit for bit, in the form the
    // face takes at run time at every level: the accurate algebra, rotation
    // and matrices; the roots' plain forms, whose bits the _c forms share;
    // slerp, exp, log, sqrt and pow's _c forms; slerp_fast and
    // from_axis_angle the plain ones; the _fast forms. A face forwarding to
    // another form, or a level's, differs somewhere.
    template<typename Type>
    void
    d_tests_forwards()
    {
        typedef djinterp::math::linalg::quaternion<Type> lq;
        typedef djinterp::math::quaternion<Type>         q;
        namespace la = djinterp::math::linalg;
        namespace ma = djinterp::math;

        for (int trial = 0; trial < 500; ++trial)
        {
            Type e[11];

            for (int i = 0; i < 11; ++i)
            {
                e[i] = static_cast<Type>(d_tests_random());
            }

            const Type             ea[4] = { e[0], e[1], e[2], e[3] };
            const Type             eb[4] = { e[4], e[5], e[6], e[7] };
            const Type             ev[3] = { e[8], e[9], e[10] };
            const Type             eo[3] = { -2 * e[8], -2 * e[9],
                                             -2 * e[10] };
            const Type             t     = static_cast<Type>(0.375);
            const Type             p     = static_cast<Type>(1.75);
            const lq               a(e[0], e[1], e[2], e[3]);
            const lq               b(e[4], e[5], e[6], e[7]);
            const q                am(e[0], e[1], e[2], e[3]);
            const q                bm(e[4], e[5], e[6], e[7]);
            const vector<Type, 3>  v(e[8], e[9], e[10]);
            const vector<Type, 3>  o(eo[0], eo[1], eo[2]);
            const vector<Type, 3>  u(e[4], e[5], e[6]);
            const Type             eu[3] = { e[4], e[5], e[6] };
            Type                   r[16];
            Type                   na[4];
            Type                   nb[4];

            // the algebra
            d_quat_multiply(r, ea, eb);
            D_TESTS_EXPECT( (d_tests_lq_is(a * b, r)) &&
                            (d_tests_mq_is(am * bm, r)),
                            "operator* forwards");
            d_quat_multiply_fast(r, ea, eb);
            D_TESTS_EXPECT( (d_tests_lq_is(la::multiply_fast(a, b), r)) &&
                            (d_tests_mq_is(ma::multiply_fast(am, bm), r)),
                            "multiply_fast forwards");
            d_quat_inverse(r, ea);
            D_TESTS_EXPECT( (d_tests_lq_is(la::inverse(a), r)) &&
                            (d_tests_mq_is(ma::inverse(am), r)),
                            "inverse forwards");
            d_quat_divide(r, ea, eb);
            D_TESTS_EXPECT( (d_tests_lq_is(a / b, r)) &&
                            (d_tests_mq_is(am / bm, r)),
                            "operator/ forwards");
            d_quat_left_divide(r, ea, eb);
            D_TESTS_EXPECT( (d_tests_lq_is(la::left_divide(a, b), r)) &&
                            (d_tests_mq_is(ma::left_divide(am, bm), r)),
                            "left_divide forwards");

            // norms
            D_TESTS_EXPECT( (d_tests_same(la::norm(a), d_quat_norm(ea))) &&
                            (d_tests_same(ma::norm(am), d_quat_norm(ea))),
                            "norm forwards");
            d_quat_normalize(na, ea);
            d_quat_normalize(nb, eb);
            D_TESTS_EXPECT( (d_tests_lq_is(la::normalize(a), na)) &&
                            (d_tests_mq_is(ma::normalize(am), na)),
                            "normalize forwards");
            d_quat_normalize_fast(r, ea);
            D_TESTS_EXPECT( (d_tests_lq_is(la::normalize_fast(a), r)) &&
                            (d_tests_mq_is(ma::normalize_fast(am), r)),
                            "normalize_fast forwards");

            // rotation and matrices
            d_quat_rotate(r, ea, ev);
            {
                const vector<Type, 3> x = la::rotate(a, v);
                const vector<Type, 3> y = ma::rotate(am, v);

                D_TESTS_EXPECT( (d_tests_same(x[0], r[0])) &&
                                (d_tests_same(x[1], r[1])) &&
                                (d_tests_same(x[2], r[2])) &&
                                (d_tests_same(y[0], r[0])) &&
                                (d_tests_same(y[1], r[1])) &&
                                (d_tests_same(y[2], r[2])),
                                "rotate forwards");
            }
            d_quat_rotate_fast(r, ea, ev);
            {
                const vector<Type, 3> x = la::rotate_fast(a, v);
                const vector<Type, 3> y = ma::rotate_fast(am, v);

                D_TESTS_EXPECT( (d_tests_same(x[0], r[0])) &&
                                (d_tests_same(x[2], r[2])) &&
                                (d_tests_same(y[1], r[1])) &&
                                (d_tests_same(y[2], r[2])),
                                "rotate_fast forwards");
            }
            d_quat_to_matrix3(r, ea);
            D_TESTS_EXPECT( (d_tests_matrix_is(la::to_matrix3(a), r)) &&
                            (d_tests_matrix_is(ma::to_matrix(am), r)),
                            "to_matrix3 forwards");
            d_quat_to_matrix3_fast(r, ea);
            D_TESTS_EXPECT( (d_tests_matrix_is(la::to_matrix3_fast(a), r)) &&
                            (d_tests_matrix_is(ma::to_matrix_fast(am), r)),
                            "to_matrix3_fast forwards");
            d_quat_to_matrix4(r, ea);
            D_TESTS_EXPECT( (d_tests_matrix_is(la::to_matrix4(a), r)) &&
                            (d_tests_matrix_is(ma::to_matrix4(am), r)),
                            "to_matrix4 forwards");
            d_quat_to_matrix4_fast(r, ea);
            D_TESTS_EXPECT( (d_tests_matrix_is(la::to_matrix4_fast(a), r)) &&
                            (d_tests_matrix_is(ma::to_matrix4_fast(am), r)),
                            "to_matrix4_fast forwards");

            // construction: Shepperd's method of any matrix, 3 x 3 and 4 x 4
            {
                const matrix<Type, 3, 3> m3 = la::to_matrix3_fast(a);
                const matrix<Type, 4, 4> m4 = la::to_matrix4_fast(b);

                d_quat_from_matrix3(r, &m3(0, 0));
                D_TESTS_EXPECT( (d_tests_lq_is(la::from_matrix(m3), r)) &&
                                (d_tests_mq_is(ma::from_matrix(m3), r)),
                                "from_matrix of a 3 x 3 forwards");
                d_quat_from_matrix4(r, &m4(0, 0));
                D_TESTS_EXPECT( (d_tests_lq_is(la::from_matrix(m4), r)) &&
                                (d_tests_mq_is(ma::from_matrix(m4), r)),
                                "from_matrix of a 4 x 4 forwards");
            }
            d_quat_from_two_vectors(r, ev, eu);
            D_TESTS_EXPECT( (d_tests_lq_is(la::from_two_vectors(v, u), r)) &&
                            (d_tests_mq_is(ma::from_two_vectors(v, u), r)),
                            "from_two_vectors forwards");
            d_quat_from_two_vectors(r, ev, eo);
            D_TESTS_EXPECT( (d_tests_lq_is(la::from_two_vectors(v, o), r)) &&
                            (d_tests_mq_is(ma::from_two_vectors(v, o), r)),
                            "from_two_vectors of opposed vectors forwards");
            d_quat_from_axis_angle(r, ev, p);
            D_TESTS_EXPECT(d_tests_lq_is(la::from_axis_angle(v, p), r),
                           "from_axis_angle forwards");

            // interpolation, of unit quaternions
            {
                const lq ua(na[0], na[1], na[2], na[3]);
                const lq ub(nb[0], nb[1], nb[2], nb[3]);
                const q  um(na[0], na[1], na[2], na[3]);
                const q  vm(nb[0], nb[1], nb[2], nb[3]);

                d_quat_nlerp(r, na, nb, t);
                D_TESTS_EXPECT( (d_tests_lq_is(la::nlerp(ua, ub, t), r)) &&
                                (d_tests_mq_is(ma::nlerp(um, vm, t), r)),
                                "nlerp forwards");
                d_quat_slerp_c(r, na, nb, t);
                D_TESTS_EXPECT( (d_tests_lq_is(la::slerp(ua, ub, t), r)) &&
                                (d_tests_mq_is(ma::slerp(um, vm, t), r)),
                                "slerp forwards to slerp_c");
                d_quat_slerp(r, na, nb, t);
                D_TESTS_EXPECT( (d_tests_lq_is(la::slerp_fast(ua, ub, t),
                                               r)) &&
                                (d_tests_mq_is(ma::slerp_fast(um, vm, t),
                                               r)),
                                "slerp_fast forwards to the plain slerp");
            }

            // exp, log, sqrt and pow: the _c forms
            d_quat_exp_c(r, ea);
            D_TESTS_EXPECT( (d_tests_lq_is(la::exp(a), r)) &&
                            (d_tests_mq_is(ma::exp(am), r)),
                            "exp forwards to exp_c");
            d_quat_log_c(r, ea);
            D_TESTS_EXPECT( (d_tests_lq_is(la::log(a), r)) &&
                            (d_tests_mq_is(ma::log(am), r)),
                            "log forwards to log_c");
            d_quat_sqrt_c(r, ea);
            D_TESTS_EXPECT( (d_tests_lq_is(la::sqrt(a), r)) &&
                            (d_tests_mq_is(ma::sqrt(am), r)),
                            "sqrt forwards to sqrt_c");
            d_quat_pow_c(r, ea, p);
            D_TESTS_EXPECT( (d_tests_lq_is(la::pow(a, p), r)) &&
                            (d_tests_mq_is(ma::pow(am, p), r)),
                            "pow forwards to pow_c");
        }

        return;
    }

    void
    d_tests_generic_parity()
    {
        typedef djinterp::math::linalg::quaternion<double> lq;
        typedef djinterp::math::linalg::quaternion<t_real> lr;
        namespace la = djinterp::math::linalg;

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
            const lq p  = la::multiply_fast(a, b);
            const lr pr = ar * br;
            const lq pa = a * b;

            // the former formulas, bit for bit
            D_TESTS_EXPECT( (d_tests_same(p.m_x, pr.m_x.v)) &&
                            (d_tests_same(p.m_y, pr.m_y.v)) &&
                            (d_tests_same(p.m_z, pr.m_z.v)) &&
                            (d_tests_same(p.m_w, pr.m_w.v)),
                            "the product, generic and the C core's fast");
            D_TESTS_EXPECT( (d_tests_near(pa.m_x, pr.m_x.v, 1e-14)) &&
                            (d_tests_near(pa.m_w, pr.m_w.v, 1e-14)),
                            "the product, generic and the C core's");
            D_TESTS_EXPECT(d_tests_near(la::norm(a), la::norm(ar).v, 1e-14),
                           "the norm, generic and C");

            // the same unit quaternion in both: the C core's normalization
            const lq na = la::normalize(a);
            const lr nr(na.m_x, na.m_y, na.m_z, na.m_w);
            const lq nb = la::normalize(b);
            const lr nbr(nb.m_x, nb.m_y, nb.m_z, nb.m_w);
            const vector<double, 3> v(e[4], e[5], e[6]);
            const t_real            vr_e[3] = { e[4], e[5], e[6] };
            const vector<t_real, 3> vr(vr_e);
            const vector<double, 3> r  = la::rotate_fast(na, v);
            const vector<t_real, 3> rr = la::rotate_fast(nr, vr);
            const vector<double, 3> ra = la::rotate(a, v);
            const vector<t_real, 3> rra = la::rotate(ar, vr);
            const matrix<double, 3, 3> m  = la::to_matrix3_fast(na);
            const matrix<t_real, 3, 3> mr = la::to_matrix3_fast(nr);
            const lq s  = la::slerp(na, nb, 0.25);
            const lr sr = la::slerp(nr, nbr, t_real(0.25));
            const lq x  = la::exp(a);
            const lr xr = la::exp(ar);

            D_TESTS_EXPECT( (d_tests_near(na.m_w, la::normalize(ar).m_w.v,
                                          1e-15)) &&
                            (d_tests_same(r[0], rr[0].v)) &&
                            (d_tests_same(r[2], rr[2].v)) &&
                            (d_tests_same(m(0, 1), mr(0, 1).v)) &&
                            (d_tests_same(m(2, 2), mr(2, 2).v)) &&
                            (d_tests_same(s.m_x, sr.m_x.v)) &&
                            (d_tests_same(s.m_w, sr.m_w.v)) &&
                            (d_tests_same(x.m_y, xr.m_y.v)) &&
                            (d_tests_same(x.m_w, xr.m_w.v)),
                            "the fast forms, slerp and exp: generic and C");
            D_TESTS_EXPECT( (d_tests_near(ra[0], rra[0].v,
                                          1e-13 * (1.0 + std::fabs(ra[0]))))
                            &&
                            (d_tests_near(la::inverse(a).m_z,
                                          la::inverse(ar).m_z.v, 1e-14)) &&
                            (d_tests_near((a / b).m_y, (ar / br).m_y.v,
                                          1e-12)) &&
                            (d_tests_near(la::from_matrix(la::to_matrix3(na))
                                              .m_w,
                                          la::from_matrix(la::to_matrix3(nr))
                                              .m_w.v,
                                          1e-14)) &&
                            (d_tests_near(la::nlerp(na, nb, 0.5).m_x,
                                          la::nlerp(nr, nbr,
                                                    t_real(0.5)).m_x.v,
                                          1e-14)),
                            "the accurate algebra: generic and C");

            // the left quotient; the accurate matrix of a q off unit
            // length; two vectors' quaternion, of opposed vectors too; and
            // log, sqrt and pow through double, bit for bit
            const vector<double, 3>    w(e[0], e[1], e[2]);
            const t_real               wr_e[3] = { e[0], e[1], e[2] };
            const vector<t_real, 3>    wr(wr_e);
            const vector<double, 3>    mw(-2 * e[0], -2 * e[1], -2 * e[2]);
            const t_real               mwr_e[3] = { -2 * e[0], -2 * e[1],
                                                    -2 * e[2] };
            const vector<t_real, 3>    mwr(mwr_e);
            const lq                   ld  = la::left_divide(a, b);
            const lr                   ldr = la::left_divide(ar, br);
            const matrix<double, 3, 3> m3  = la::to_matrix3(a);
            const matrix<t_real, 3, 3> m3r = la::to_matrix3(ar);
            const lq                   tv  = la::from_two_vectors(w, v);
            const lr                   tvr = la::from_two_vectors(wr, vr);
            const lq                   to  = la::from_two_vectors(w, mw);
            const lr                   tor = la::from_two_vectors(wr, mwr);
            const lq                   lg  = la::log(a);
            const lr                   lgr = la::log(ar);
            const lq                   sq  = la::sqrt(a);
            const lr                   sqr = la::sqrt(ar);
            const lq                   pw  = la::pow(a, 1.75);
            const lr                   pwr = la::pow(ar, t_real(1.75));
            const lq                   fm  = la::from_matrix(m3);
            const lr                   fmr = la::from_matrix(m3r);
            bool                       matrix_near = true;

            for (std::size_t i = 0; i < 3; ++i)
            {
                for (std::size_t j = 0; j < 3; ++j)
                {
                    matrix_near = matrix_near &&
                                  d_tests_near(m3(i, j), m3r(i, j).v, 1e-14);
                }
            }

            D_TESTS_EXPECT( (d_tests_near(ld.m_x, ldr.m_x.v, 1e-12)) &&
                            (d_tests_near(ld.m_w, ldr.m_w.v, 1e-12)) &&
                            (matrix_near) &&
                            (d_tests_near(tv.m_y, tvr.m_y.v, 1e-12)) &&
                            (d_tests_near(tv.m_w, tvr.m_w.v, 1e-12)) &&
                            (d_tests_near(to.m_x, tor.m_x.v, 1e-15)) &&
                            (d_tests_near(to.m_y, tor.m_y.v, 1e-15)) &&
                            (d_tests_near(to.m_z, tor.m_z.v, 1e-15)) &&
                            (d_tests_near(to.m_w, tor.m_w.v, 1e-15)),
                            "the left quotient, the matrix and two vectors: "
                            "generic and C");
            D_TESTS_EXPECT( (d_tests_near(fm.m_x, fmr.m_x.v, 1e-14)) &&
                            (d_tests_near(fm.m_y, fmr.m_y.v, 1e-14)) &&
                            (d_tests_near(fm.m_z, fmr.m_z.v, 1e-14)) &&
                            (d_tests_near(fm.m_w, fmr.m_w.v, 1e-14)),
                            "Shepperd's method of a q off unit length's "
                            "matrix: generic and C");
            D_TESTS_EXPECT( (d_tests_same(lg.m_x, lgr.m_x.v)) &&
                            (d_tests_same(lg.m_w, lgr.m_w.v)) &&
                            (d_tests_same(sq.m_y, sqr.m_y.v)) &&
                            (d_tests_same(sq.m_w, sqr.m_w.v)) &&
                            (d_tests_same(pw.m_z, pwr.m_z.v)) &&
                            (d_tests_same(pw.m_w, pwr.m_w.v)),
                            "log, sqrt and pow: generic and C");
        }

        // a zero vector names no rotation: the identity, generic too
        {
            const t_real            zero_e[3] = { 0.0, 0.0, 0.0 };
            const t_real            some_e[3] = { 1.0, -2.0, 3.0 };
            const vector<t_real, 3> zero(zero_e);
            const vector<t_real, 3> some(some_e);
            const lr                id = la::from_two_vectors(zero, some);

            D_TESTS_EXPECT( (id.m_x.v == 0.0) && (id.m_y.v == 0.0) &&
                            (id.m_z.v == 0.0) && (id.m_w.v == 1.0),
                            "two vectors, one 0: the identity, generic");
        }

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
namespace
{
    typedef djinterp::math::linalg::quaternion<double> cq;
    typedef djinterp::math::linalg::vector<double, 3>  cv;

    constexpr cq g_cq(0.0, 0.0, 0.6, 0.8);
    constexpr cq g_cp(0.5, -0.5, 0.5, 0.5);
}  // namespace

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
static_assert(djinterp::math::linalg::inverse(cq(0, 0, 0, 2)).m_w == 0.5,
              "a constexpr inverse");
static_assert((g_cp / g_cp).m_w == 1.0, "a constexpr quotient");
static_assert(djinterp::math::linalg::left_divide(g_cp, g_cp).m_w == 1.0,
              "a constexpr left quotient");
static_assert(djinterp::math::linalg::rotate(g_cq, cv(1, 0, 0))[2] == 0.0,
              "a constexpr rotation");
static_assert(djinterp::math::linalg::to_matrix3(g_cq)(2, 2) == 1.0,
              "a constexpr matrix");
static_assert(djinterp::math::linalg::from_matrix(
                  djinterp::math::linalg::to_matrix3(g_cp)).m_w == 0.5,
              "a constexpr Shepperd's method");
static_assert(djinterp::math::linalg::from_two_vectors(cv(1, 0, 0),
                                                       cv(1, 0, 0)).m_w ==
                  1.0,
              "a constexpr two vectors");
static_assert(djinterp::math::linalg::nlerp(g_cq, g_cq, 0.5).m_z ==
                  g_cq.m_z,
              "a constexpr nlerp");
static_assert(djinterp::math::linalg::slerp(g_cq, g_cp, 0.0).m_z ==
                  g_cq.m_z,
              "a constexpr slerp");
static_assert(djinterp::math::linalg::exp(cq(0, 0, 0, 0)).m_w == 1.0,
              "a constexpr exp");
static_assert(djinterp::math::linalg::log(cq(0, 0, 0, 1)).m_w == 0.0,
              "a constexpr log");
static_assert(djinterp::math::linalg::sqrt(cq(0, 0, 0, 4)).m_w == 2.0,
              "a constexpr sqrt");
static_assert(djinterp::math::linalg::pow(cq(0, 0, 0, 2), 3.0).m_w == 8.0,
              "a constexpr pow");
static_assert(djinterp::math::exp(djinterp::math::quaternion<double>(
                  0.0, 0.0, 0.0, 0.0)).w == 1.0,
              "math's constexpr exp");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert(djinterp::math::linalg::from_axis_angle(
                  vector<double, 3>(0, 0, 1), 0.0).m_w == 1.0,
              "a constexpr rotation from an axis and angle");
static_assert(djinterp::math::linalg::slerp_fast(
                  djinterp::math::linalg::quaternion<double>(),
                  djinterp::math::linalg::quaternion<double>(), 0.5).m_w ==
                  1.0,
              "a constexpr slerp_fast");
#endif


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);
    d_tests_forwards<float>();
    d_tests_forwards<double>();
    d_tests_forwards<long double>();
    d_tests_generic_parity();

    (void)sizeof(mq);

    std::printf("t_quaternion_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
