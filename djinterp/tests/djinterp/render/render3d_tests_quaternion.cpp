/*******************************************************************************
* djinterp [render]                                render3d_tests_quaternion.cpp
*
* Unit tests for math::linalg::quaternion, re-exported by render3d.
*
*
* path:      /tests/djinterp/render/render3d_tests_quaternion.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "render3d_tests.hpp"  // corresponding header


namespace djinterp
{
namespace testing
{

/*
tests_quaternion_rotation
  rotation about an axis, and the three ways of applying it agreeing
  Tests the following:
  - a quarter turn about z takes x to y
  - to_matrix3 and to_matrix4 rotate exactly as rotate() does
  - composition: (b * a) applies a, then b
  - the conjugate undoes the rotation
  - the axis is normalized; a zero axis gives the identity
*/
bool
tests_quaternion_rotation()
{
    const double half_pi = 1.5707963267948966;

    const la::quaternion<double> quarter_z =
        la::from_axis_angle(vec3(0.0, 0.0, 1.0), half_pi);

    if (!near_vec(la::rotate(quarter_z, vec3(1.0, 0.0, 0.0)),
                  vec3(0.0, 1.0, 0.0)))
    {
        return false;
    }

    // the matrices rotate the same way as the quaternion
    const vec3                   v(0.3, -1.2, 2.5);
    const la::quaternion<double> q =
        la::from_axis_angle(vec3(1.0, 2.0, -0.5), 0.9);
    const la::matrix<double, 3, 3> m3 = la::to_matrix3(q);
    const la::matrix<double, 4, 4> m4 = la::to_matrix4(q);

    const vec3 by_m3 = m3 * v;
    const la::vector<double, 4> by_m4 =
        m4 * la::vector<double, 4>(v[0], v[1], v[2], 1.0);

    if ( (!near_vec(by_m3, la::rotate(q, v))) ||
         (!near_vec(vec3(by_m4[0], by_m4[1], by_m4[2]), by_m3)) ||
         (!near(by_m4[3], 1.0)) )
    {
        return false;
    }

    // b * a applies a first: a quarter turn about z, then about x
    const la::quaternion<double> quarter_x =
        la::from_axis_angle(vec3(1.0, 0.0, 0.0), half_pi);
    const vec3 twice = la::rotate(quarter_x * quarter_z, vec3(1.0, 0.0, 0.0));

    if (!near_vec(twice, la::rotate(quarter_x,
                                    la::rotate(quarter_z,
                                               vec3(1.0, 0.0, 0.0)))))
    {
        return false;
    }

    if (!near_vec(twice, vec3(0.0, 0.0, 1.0)))
    {
        return false;
    }

    // the conjugate rotates back
    if (!near_vec(la::rotate(la::conjugate(q), la::rotate(q, v)), v))
    {
        return false;
    }

    // a long axis is normalized; a zero axis is the identity
    const la::quaternion<double> scaled =
        la::from_axis_angle(vec3(0.0, 0.0, 7.0), half_pi);
    const la::quaternion<double> none =
        la::from_axis_angle(vec3(0.0, 0.0, 0.0), half_pi);

    return ( (near(la::norm(scaled), 1.0)) &&
             (near_vec(la::rotate(scaled, vec3(1.0, 0.0, 0.0)),
                       vec3(0.0, 1.0, 0.0))) &&
             (near_vec(la::rotate(none, v), v)) );
}

/*
tests_quaternion_slerp
  interpolation along the shorter arc
  Tests the following:
  - t = 0 and t = 1 give the endpoints
  - halfway between 0 and 90 degrees about z is 45 degrees
  - the negated end quaternion (the same rotation) takes the same path
  - nearly equal endpoints fall back to normalized linear interpolation
*/
bool
tests_quaternion_slerp()
{
    const double half_pi = 1.5707963267948966;
    const vec3   z(0.0, 0.0, 1.0);
    const vec3   x(1.0, 0.0, 0.0);

    const la::quaternion<double> a = la::quaternion<double>::identity();
    const la::quaternion<double> b = la::from_axis_angle(z, half_pi);

    if ( (!near_vec(la::rotate(la::slerp(a, b, 0.0), x), x)) ||
         (!near_vec(la::rotate(la::slerp(a, b, 1.0), x),
                    vec3(0.0, 1.0, 0.0))) )
    {
        return false;
    }

    const double s = 0.7071067811865476;

    if (!near_vec(la::rotate(la::slerp(a, b, 0.5), x), vec3(s, s, 0.0)))
    {
        return false;
    }

    // -b is the same rotation; slerp must not go the long way round
    const la::quaternion<double> neg_b(-b.m_x, -b.m_y, -b.m_z, -b.m_w);

    if (!near_vec(la::rotate(la::slerp(a, neg_b, 0.5), x), vec3(s, s, 0.0)))
    {
        return false;
    }

    // almost no angle between the endpoints: still a unit quaternion
    const la::quaternion<double> tiny = la::from_axis_angle(z, 1e-6);

    return near(la::norm(la::slerp(a, tiny, 0.5)), 1.0, 1e-12);
}

}  // namespace testing
}  // namespace djinterp
