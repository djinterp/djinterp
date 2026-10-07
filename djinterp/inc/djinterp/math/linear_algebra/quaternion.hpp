/*******************************************************************************
* djinterp [math]                                                 quaternion.hpp
*
* Quaternions for rotation in three dimensions.
*   A quaternion x i + y j + z k + w, stored as its vector part (x, y, z) and
* scalar part w. Unit quaternions represent rotations: they compose by the
* Hamilton product, rotate linalg vectors, convert to 3x3 and 4x4 rotation
* matrices, and interpolate along the shorter arc with slerp. The default value
* is the identity rotation.
*   CONVENTION: matching linalg::matrix and linalg::transform_point, the
* matrices are row-major for column vectors -- a point is rotated as M * p.
*
*
* path:      /inc/djinterp/math/linear_algebra/quaternion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE QUATERNION
    --------------
    1.  quaternion
2.  OPERATIONS
    ----------
    1.  Construction
         1.  from_axis_angle
    2.  Algebra
         1.  operator*
         2.  conjugate
         3.  norm
         4.  normalize
    3.  Rotation
         1.  rotate
         2.  to_matrix3
         3.  to_matrix4
    4.  Interpolation
         1.  slerp
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP 1

// std
#include <cmath>  // std::sqrt, std::sin, std::cos, std::acos
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "./matrix.hpp"        // matrix<T, 3, 3>, matrix<T, 4, 4>
#include "./vector.hpp"        // vector<T, 3>, cross


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  THE QUATERNION
//==============================================================================


// 1.1    quaternion
//------------------------------------------------------------------------------
// 1.1.1
// quaternion
//   struct: x i + y j + z k + w. Default-constructs to the identity rotation.
template<typename T = double>
struct quaternion
{
    using value_type  = T;
    using vector_type = vector<T, 3>;

    T m_x;
    T m_y;
    T m_z;
    T m_w;

    D_CONSTEXPR quaternion() noexcept
        : m_x(static_cast<T>(0)),
          m_y(static_cast<T>(0)),
          m_z(static_cast<T>(0)),
          m_w(static_cast<T>(1))
    {}

    D_CONSTEXPR quaternion(
        T _x,
        T _y,
        T _z,
        T _w
    ) noexcept
        : m_x(_x),
          m_y(_y),
          m_z(_z),
          m_w(_w)
    {}

    // identity
    //   the rotation that leaves every vector unchanged.
    D_NODISCARD static D_CONSTEXPR quaternion
    identity() noexcept
    {
        return quaternion();
    }

    // vector_part
    //   (x, y, z) as a linalg vector.
    D_NODISCARD D_CONSTEXPR vector_type
    vector_part() const noexcept
    {
        return vector_type(m_x, m_y, m_z);
    }
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Construction
//------------------------------------------------------------------------------
// 2.1.1
// from_axis_angle
//   function: the rotation of _angle radians about _axis. The axis is
// normalized here, so any non-zero length will do; a zero axis gives the
// identity.
template<typename T>
D_NODISCARD quaternion<T>
from_axis_angle(
    const vector<T, 3>& _axis,
    T                   _angle
) noexcept
{
    const T length = _axis.length();

    // a zero axis names no rotation
    if (length == static_cast<T>(0))
    {
        return quaternion<T>::identity();
    }

    const T half = _angle / static_cast<T>(2);
    const T s    = std::sin(half) / length;

    return quaternion<T>(_axis[0] * s,
                          _axis[1] * s,
                          _axis[2] * s,
                          std::cos(half));
}


// 2.2    Algebra
//------------------------------------------------------------------------------
// 2.2.1
// operator*
//   function: the Hamilton product; _a * _b rotates by _b, then by _a.
template<typename T>
D_NODISCARD D_CONSTEXPR quaternion<T>
operator*(
    const quaternion<T>& _a,
    const quaternion<T>& _b
) noexcept
{
    return quaternion<T>(
        (_a.m_w * _b.m_x) + (_a.m_x * _b.m_w) + (_a.m_y * _b.m_z) -
            (_a.m_z * _b.m_y),
        (_a.m_w * _b.m_y) - (_a.m_x * _b.m_z) + (_a.m_y * _b.m_w) +
            (_a.m_z * _b.m_x),
        (_a.m_w * _b.m_z) + (_a.m_x * _b.m_y) - (_a.m_y * _b.m_x) +
            (_a.m_z * _b.m_w),
        (_a.m_w * _b.m_w) - (_a.m_x * _b.m_x) - (_a.m_y * _b.m_y) -
            (_a.m_z * _b.m_z));
}

// 2.2.2
// conjugate
//   function: (-x, -y, -z, w); the inverse of a unit quaternion.
template<typename T>
D_NODISCARD D_CONSTEXPR quaternion<T>
conjugate(
    const quaternion<T>& _q
) noexcept
{
    return quaternion<T>(-_q.m_x, -_q.m_y, -_q.m_z, _q.m_w);
}

// 2.2.3
// norm
//   function: the Euclidean norm of (x, y, z, w).
template<typename T>
D_NODISCARD T
norm(
    const quaternion<T>& _q
) noexcept
{
    return std::sqrt((_q.m_x * _q.m_x) + (_q.m_y * _q.m_y) +
                     (_q.m_z * _q.m_z) + (_q.m_w * _q.m_w));
}

// 2.2.4
// normalize
//   function: _q scaled to unit norm; a zero quaternion is returned as is.
template<typename T>
D_NODISCARD quaternion<T>
normalize(
    const quaternion<T>& _q
) noexcept
{
    const T n = norm(_q);

    // there is no direction to keep in a zero quaternion
    if (n == static_cast<T>(0))
    {
        return _q;
    }

    return quaternion<T>(_q.m_x / n, _q.m_y / n, _q.m_z / n, _q.m_w / n);
}


// 2.3    Rotation
//------------------------------------------------------------------------------
// 2.3.1
// rotate
//   function: _v rotated by the unit quaternion _q.
template<typename T>
D_NODISCARD D_CONSTEXPR vector<T, 3>
rotate(
    const quaternion<T>& _q,
    const vector<T, 3>&  _v
) noexcept
{
    // v + 2w (u x v) + 2 u x (u x v), without building a matrix
    const vector<T, 3> u = _q.vector_part();
    const vector<T, 3> t = cross(u, _v) * static_cast<T>(2);

    return _v + (t * _q.m_w) + cross(u, t);
}

// 2.3.2
// to_matrix3
//   function: the 3x3 rotation matrix of the unit quaternion _q.
template<typename T>
D_NODISCARD D_CONSTEXPR matrix<T, 3, 3>
to_matrix3(
    const quaternion<T>& _q
) noexcept
{
    const T x   = _q.m_x;
    const T y   = _q.m_y;
    const T z   = _q.m_z;
    const T w   = _q.m_w;
    const T one = static_cast<T>(1);
    const T two = static_cast<T>(2);

    matrix<T, 3, 3> r = matrix<T, 3, 3>::identity();

    r(0, 0) = one - (two * ((y * y) + (z * z)));
    r(0, 1) = two * ((x * y) - (w * z));
    r(0, 2) = two * ((x * z) + (w * y));
    r(1, 0) = two * ((x * y) + (w * z));
    r(1, 1) = one - (two * ((x * x) + (z * z)));
    r(1, 2) = two * ((y * z) - (w * x));
    r(2, 0) = two * ((x * z) - (w * y));
    r(2, 1) = two * ((y * z) + (w * x));
    r(2, 2) = one - (two * ((x * x) + (y * y)));

    return r;
}

// 2.3.3
// to_matrix4
//   function: the 4x4 homogeneous rotation matrix of the unit quaternion _q.
template<typename T>
D_NODISCARD D_CONSTEXPR matrix<T, 4, 4>
to_matrix4(
    const quaternion<T>& _q
) noexcept
{
    const matrix<T, 3, 3> r3 = to_matrix3(_q);
    matrix<T, 4, 4>       r  = matrix<T, 4, 4>::identity();

    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            r(i, j) = r3(i, j);
        }
    }

    return r;
}


// 2.4    Interpolation
//------------------------------------------------------------------------------
// 2.4.1
// slerp
//   function: spherical linear interpolation from _a (t = 0) to _b (t = 1)
// along the shorter arc; both are unit quaternions.
template<typename T>
D_NODISCARD quaternion<T>
slerp(
    const quaternion<T>& _a,
    const quaternion<T>& _b,
    T                    _t
) noexcept
{
    T d = (_a.m_x * _b.m_x) + (_a.m_y * _b.m_y) +
           (_a.m_z * _b.m_z) + (_a.m_w * _b.m_w);
    quaternion<T> b = _b;

    // q and -q are the same rotation; take the one on the shorter arc
    if (d < static_cast<T>(0))
    {
        b = quaternion<T>(-_b.m_x, -_b.m_y, -_b.m_z, -_b.m_w);
        d = -d;
    }

    // nearly parallel: the arc is a line, and sin(theta) would vanish
    if (d > static_cast<T>(0.9995))
    {
        return normalize(quaternion<T>(_a.m_x + (_t * (b.m_x - _a.m_x)),
                                        _a.m_y + (_t * (b.m_y - _a.m_y)),
                                        _a.m_z + (_t * (b.m_z - _a.m_z)),
                                        _a.m_w + (_t * (b.m_w - _a.m_w))));
    }

    const T theta_0 = std::acos(d);
    const T theta   = theta_0 * _t;
    const T sin_0   = std::sin(theta_0);
    const T s_a     = std::sin(theta_0 - theta) / sin_0;
    const T s_b     = std::sin(theta) / sin_0;

    return quaternion<T>((s_a * _a.m_x) + (s_b * b.m_x),
                          (s_a * _a.m_y) + (s_b * b.m_y),
                          (s_a * _a.m_z) + (s_b * b.m_z),
                          (s_a * _a.m_w) + (s_b * b.m_w));
}

}  // namespace linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
