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
*   A face over the C core, c/math/quat.h, for float, double and long double
* (internal::linalg_kernel, which copies the four members in and out); any
* other element type takes the generic path. Everything compiles from C++98.
* The algebra, the norm and the rotations are constant expressions from
* C++14; from_axis_angle and slerp, which need a sine and a cosine, take the
* C library's at run time at every level and are constant expressions from
* C++20.
*
*
* path:      /inc/djinterp/math/linear_algebra/quaternion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.10.05
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
#include <cstddef>                 // std::size_t
// djinterp
#include "../../djinterp.hpp"      // framework root
#include "./linalg_common.hpp"     // internal::linalg_kernel
#include "./matrix.hpp"            // matrix<T, 3, 3>, matrix<T, 4, 4>
#include "./vector.hpp"            // vector<T, 3>


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
    typedef T            value_type;
    typedef vector<T, 3> vector_type;

    T m_x;
    T m_y;
    T m_z;
    T m_w;

    D_CONSTEXPR quaternion() D_NOEXCEPT
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
    ) D_NOEXCEPT
        : m_x(_x),
          m_y(_y),
          m_z(_z),
          m_w(_w)
    {}

    // identity
    //   the rotation that leaves every vector unchanged.
    D_NODISCARD static D_CONSTEXPR quaternion
    identity() D_NOEXCEPT
    {
        return quaternion();
    }

    // vector_part
    //   (x, y, z) as a linalg vector.
    D_NODISCARD D_CONSTEXPR_CPP14 vector_type
    vector_part() const D_NOEXCEPT
    {
        return vector_type(m_x, m_y, m_z);
    }
};

NS_INTERNAL

    // quat_in, quat_out
    //   function: a quaternion as the C core's array [x, y, z, w], and back.
    template<typename T>
    D_CONSTEXPR_CPP14 void
    quat_in(
        const quaternion<T>& _q,
        T*                   _out
    ) D_NOEXCEPT
    {
        _out[0] = _q.m_x;
        _out[1] = _q.m_y;
        _out[2] = _q.m_z;
        _out[3] = _q.m_w;
    }

    template<typename T>
    D_CONSTEXPR_CPP14 quaternion<T>
    quat_out(const T* _q) D_NOEXCEPT
    {
        return quaternion<T>(_q[0], _q[1], _q[2], _q[3]);
    }

NS_END  // internal


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
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<T>
from_axis_angle(
    const vector<T, 3>& _axis,
    T                   _angle
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::linalg_kernel<T>::quat_from_axis_angle(q,
                                                     _axis.data(),
                                                     _angle);

    return internal::quat_out(q);
}

// 2.2    Algebra
//------------------------------------------------------------------------------
// 2.2.1
// operator*
//   function: the Hamilton product; _a * _b rotates by _b, then by _a.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
operator*(
    const quaternion<T>& _a,
    const quaternion<T>& _b
) D_NOEXCEPT
{
    T a[4] = { T(), T(), T(), T() };
    T b[4] = { T(), T(), T(), T() };

    internal::quat_in(_a, a);
    internal::quat_in(_b, b);
    internal::linalg_kernel<T>::quat_multiply(a, a, b);

    return internal::quat_out(a);
}

// 2.2.2
// conjugate
//   function: (-x, -y, -z, w); the inverse of a unit quaternion.
template<typename T>
D_NODISCARD D_CONSTEXPR quaternion<T>
conjugate(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    return quaternion<T>(-_q.m_x, -_q.m_y, -_q.m_z, _q.m_w);
}

// 2.2.3
// norm
//   function: the Euclidean norm of (x, y, z, w), correctly rounded.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 T
norm(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);

    return internal::linalg_kernel<T>::norm(q, 4);
}

// 2.2.4
// normalize
//   function: _q scaled to unit norm; a zero quaternion is returned as is.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
normalize(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_normalize(q, q);

    return internal::quat_out(q);
}

// 2.3    Rotation
//------------------------------------------------------------------------------
// 2.3.1
// rotate
//   function: _v rotated by the unit quaternion _q, v + w t + u x t with t =
// 2 u x v, without building a matrix.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 vector<T, 3>
rotate(
    const quaternion<T>& _q,
    const vector<T, 3>&  _v
) D_NOEXCEPT
{
    T            q[4] = { T(), T(), T(), T() };
    vector<T, 3> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_rotate(&r[0], q, _v.data());

    return r;
}

// 2.3.2
// to_matrix3
//   function: the 3x3 rotation matrix of the unit quaternion _q.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 matrix<T, 3, 3>
to_matrix3(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T               q[4] = { T(), T(), T(), T() };
    matrix<T, 3, 3> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_to_matrix3(&r(0, 0), q);

    return r;
}

// 2.3.3
// to_matrix4
//   function: the 4x4 homogeneous rotation matrix of the unit quaternion _q.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 matrix<T, 4, 4>
to_matrix4(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T               q[4] = { T(), T(), T(), T() };
    matrix<T, 4, 4> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_to_matrix4(&r(0, 0), q);

    return r;
}

// 2.4    Interpolation
//------------------------------------------------------------------------------
// 2.4.1
// slerp
//   function: spherical linear interpolation from _a (t = 0) to _b (t = 1)
// along the shorter arc; both are unit quaternions.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<T>
slerp(
    const quaternion<T>& _a,
    const quaternion<T>& _b,
    T                    _t
) D_NOEXCEPT
{
    T a[4] = { T(), T(), T(), T() };
    T b[4] = { T(), T(), T(), T() };
    T r[4] = { T(), T(), T(), T() };

    internal::quat_in(_a, a);
    internal::quat_in(_b, b);
    internal::linalg_kernel<T>::quat_slerp(r, a, b, _t);

    return internal::quat_out(r);
}


}  // namespace linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
