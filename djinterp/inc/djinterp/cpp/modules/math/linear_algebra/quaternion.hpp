/*******************************************************************************
* djinterp [cpp/math]                                             quaternion.hpp
*
* Quaternions for rotation in three dimensions, and their algebra.
*   A quaternion x i + y j + z k + w, stored as its vector part (x, y, z) and
* scalar part w. Unit quaternions represent rotations: they compose by the
* Hamilton product, rotate linalg vectors, convert to 3x3 and 4x4 rotation
* matrices and back, and interpolate along the shorter arc with slerp and
* nlerp; the inverse and the two quotients, exp, log, sqrt and pow (S275)
* are the algebra's. The default value is the identity rotation.
*   CONVENTION: matching linalg::matrix and linalg::transform_point, the
* matrices are row-major for column vectors -- a point is rotated as M * p;
* a rotation is active and right-handed, q v q^-1, as glTF's, Eigen's and
* GLM's (S276).
*   A face over the C core, c/math/quat.h, for float, double and long double
* (internal::linalg_kernel, which copies the four members in and out); any
* other element type takes the generic path. Everything compiles from C++98.
* Accurate by default (S276): each part of the product, the inverse, the
* quotients, the rotation and its matrix within a step of the correctly
* rounded one, the rotation and the matrix of any q not 0, a unit q's own;
* the _fast forms are the former formulas, for a renderer's inner loops.
* Everything but from_axis_angle, slerp_fast and normalize_fast is a
* constant expression from C++14; those two take the C library's sine and
* cosine at run time, constant expressions from C++20.
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/quaternion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.11.07
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
         2.  from_matrix
         3.  from_two_vectors
    2.  Algebra
         1.  operator*, multiply_fast
         2.  inverse, operator/, left_divide
         3.  conjugate
         4.  norm
         5.  normalize, normalize_fast
    3.  Rotation
         1.  rotate, rotate_fast
         2.  to_matrix3, to_matrix3_fast
         3.  to_matrix4, to_matrix4_fast
    4.  Interpolation
         1.  slerp, slerp_fast
         2.  nlerp
    5.  Exponential, logarithm and powers
         1.  exp, log, sqrt
         2.  pow
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP 1

// std
#include <cstddef>                 // std::size_t
// djinterp
#include "../../../djinterp.hpp"   // framework root
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

// 2.1.2
// from_matrix
//   function: the unit quaternion, w >= 0, of the rotation matrix _m, by
// Shepperd's method -- to_matrix3's inverse, q or -q; of a 4x4 matrix, its
// upper-left 3x3. A matrix a few steps from orthogonal gives a quaternion
// as many from unit length.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
from_matrix(
    const matrix<T, 3, 3>& _m
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::linalg_kernel<T>::quat_from_matrix3(q, &_m(0, 0));

    return internal::quat_out(q);
}

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
from_matrix(
    const matrix<T, 4, 4>& _m
) D_NOEXCEPT
{
    const T m[9] = { _m(0, 0), _m(0, 1), _m(0, 2),
                     _m(1, 0), _m(1, 1), _m(1, 2),
                     _m(2, 0), _m(2, 1), _m(2, 2) };
    T       q[4] = { T(), T(), T(), T() };

    internal::linalg_kernel<T>::quat_from_matrix3(q, m);

    return internal::quat_out(q);
}

// 2.1.3
// from_two_vectors
//   function: the unit quaternion of the shortest rotation turning _a's
// direction into _b's; exactly opposed, a half turn about an axis
// perpendicular to _a; a zero vector, the identity.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
from_two_vectors(
    const vector<T, 3>& _a,
    const vector<T, 3>& _b
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::linalg_kernel<T>::quat_from_two_vectors(q, _a.data(),
                                                      _b.data());

    return internal::quat_out(q);
}

// 2.2    Algebra
//------------------------------------------------------------------------------
// 2.2.1
// operator*, multiply_fast
//   function: the Hamilton product; _a * _b rotates by _b, then by _a. Each
// part within a step of the correctly rounded one, as accurate as twice the
// precision (S276); multiply_fast by the former formula, for a renderer.
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

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
multiply_fast(
    const quaternion<T>& _a,
    const quaternion<T>& _b
) D_NOEXCEPT
{
    T a[4] = { T(), T(), T(), T() };
    T b[4] = { T(), T(), T(), T() };

    internal::quat_in(_a, a);
    internal::quat_in(_b, b);
    internal::linalg_kernel<T>::quat_multiply_fast(a, a, b);

    return internal::quat_out(a);
}

// 2.2.2
// inverse, operator/, left_divide
//   function: 1 / _q = q* / |q|^2, NaNs of 0; _a / _b = a b^-1, the quotient
// on the right (S277); left_divide(_a, _b) = a^-1 b, the one on the left.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
inverse(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_inverse(q, q);

    return internal::quat_out(q);
}

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
operator/(
    const quaternion<T>& _a,
    const quaternion<T>& _b
) D_NOEXCEPT
{
    T a[4] = { T(), T(), T(), T() };
    T b[4] = { T(), T(), T(), T() };

    internal::quat_in(_a, a);
    internal::quat_in(_b, b);
    internal::linalg_kernel<T>::quat_divide(a, a, b);

    return internal::quat_out(a);
}

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
left_divide(
    const quaternion<T>& _a,
    const quaternion<T>& _b
) D_NOEXCEPT
{
    T a[4] = { T(), T(), T(), T() };
    T b[4] = { T(), T(), T(), T() };

    internal::quat_in(_a, a);
    internal::quat_in(_b, b);
    internal::linalg_kernel<T>::quat_left_divide(a, a, b);

    return internal::quat_out(a);
}

// 2.2.3
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

// 2.2.4
// norm
//   function: the Euclidean norm of (x, y, z, w), within a step of the
// correctly rounded value, scaled so that it overflows only where it does.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 T
norm(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);

    return internal::linalg_kernel<T>::quat_norm(q);
}

// 2.2.5
// normalize, normalize_fast
//   function: _q scaled to unit norm, each part within a step of q / |q|, a
// zero quaternion returned as it is; normalize_fast by the reciprocal square
// root, unscaled, for a renderer.
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

template<typename T>
D_NODISCARD quaternion<T>
normalize_fast(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_normalize_fast(q, q);

    return internal::quat_out(q);
}

// 2.3    Rotation
//------------------------------------------------------------------------------
// 2.3.1
// rotate, rotate_fast
//   function: _v rotated by _q, q v q^-1, any q not 0 naming the rotation of
// q / |q|, each part within a step of the correctly rounded one; and by the
// unit quaternion _q by the former formula, v + w t + u x t, t = 2 u x v.
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

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 vector<T, 3>
rotate_fast(
    const quaternion<T>& _q,
    const vector<T, 3>&  _v
) D_NOEXCEPT
{
    T            q[4] = { T(), T(), T(), T() };
    vector<T, 3> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_rotate_fast(&r[0], q, _v.data());

    return r;
}

// 2.3.2
// to_matrix3, to_matrix3_fast
//   function: the 3x3 rotation matrix of _q, any q not 0, each entry within a
// step of the correctly rounded one; and of the unit quaternion _q by the
// former formula, 1 - 2 (y^2 + z^2) and so on.
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

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 matrix<T, 3, 3>
to_matrix3_fast(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T               q[4] = { T(), T(), T(), T() };
    matrix<T, 3, 3> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_to_matrix3_fast(&r(0, 0), q);

    return r;
}

// 2.3.3
// to_matrix4, to_matrix4_fast
//   function: the 4x4 homogeneous rotation matrix, to_matrix3's or
// to_matrix3_fast's in its upper left.
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

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 matrix<T, 4, 4>
to_matrix4_fast(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T               q[4] = { T(), T(), T(), T() };
    matrix<T, 4, 4> r;

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_to_matrix4_fast(&r(0, 0), q);

    return r;
}

// 2.4    Interpolation
//------------------------------------------------------------------------------
// 2.4.1
// slerp, slerp_fast
//   function: spherical linear interpolation from _a (t = 0) to _b (t = 1)
// along the shorter arc at a constant speed, both unit quaternions: the
// angle 2 atan2(|a - b|, |a + b|), accurate near 0, and _a at t = 0, _b or
// -_b at t = 1, exactly; slerp_fast the same in T's own precision on the C
// library's functions, a few steps of the largest part.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
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

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<T>
slerp_fast(
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
    internal::linalg_kernel<T>::quat_slerp_fast(r, a, b, _t);

    return internal::quat_out(r);
}

// 2.4.2
// nlerp
//   function: (1 - t) _a + t _b along the shorter arc, normalized: the cheap
// curve between two rotations, its speed varying along it.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
nlerp(
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
    internal::linalg_kernel<T>::quat_nlerp(r, a, b, _t);

    return internal::quat_out(r);
}

// 2.5    Exponential, logarithm and powers
//------------------------------------------------------------------------------
// The complex functions in q's plane, principal values (S275): f(q) = Re f(w
// + r i) + u Im f(w + r i), r = |v|, u = v / r; for v along x alone, the
// complex function of w + x i itself. log(-1) is pi i.

// 2.5.1
// exp, log, sqrt
//   function: e^q; the principal logarithm, log |q| + u atan2(|v|, w); the
// principal square root, its real part >= 0.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
exp(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_exp(q, q);

    return internal::quat_out(q);
}

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
log(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_log(q, q);

    return internal::quat_out(q);
}

template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
sqrt(
    const quaternion<T>& _q
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_sqrt(q, q);

    return internal::quat_out(q);
}

// 2.5.2
// pow
//   function: q^_p = exp(p log q) for a real exponent _p, the principal
// power.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<T>
pow(
    const quaternion<T>& _q,
    T                    _p
) D_NOEXCEPT
{
    T q[4] = { T(), T(), T(), T() };

    internal::quat_in(_q, q);
    internal::linalg_kernel<T>::quat_pow(q, q, _p);

    return internal::quat_out(q);
}


}  // namespace linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
