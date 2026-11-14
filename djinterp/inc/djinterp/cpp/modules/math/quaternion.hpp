/*******************************************************************************
* djinterp [cpp/math]                                             quaternion.hpp
*
* Quaternions for representing and composing 3D rotations without gimbal
* lock, and their algebra.
*   quaternion<Type> stores the vector part (x, y, z) and scalar part w of
* x i + y j + z k + w. Construction is from an axis and angle, a rotation
* matrix or two vectors; the Hamilton product composes rotations, and the
* inverse and the quotients undo them; a vector is rotated by q v q^-1; and
* the rotation converts to a 3x3 (or homogeneous 4x4) linalg matrix. slerp
* blends two orientations along the shorter arc at a constant speed, nlerp
* cheaply; exp, log, sqrt and pow are the principal values (S275).
*
* BRIDGE TO LINEAR ALGEBRA:
*   axes and rotated vectors are linalg::vector<Type, 3>; to_matrix /
* to_matrix4 yield linalg::matrix values and from_matrix takes them, so a
* quaternion drops straight into the same transform pipeline as
* linalg::transform's rotation builders.
*
* THE C CORE:
*   a face over c/math/quat.h, as linear_algebra/quaternion.hpp is, through
* the same internal::linalg_kernel; the generic path serves any other
* element type. Everything compiles from C++98. Accurate by default (S276):
* each part of the product, the inverse, the quotients, the rotation and its
* matrix within a step of the correctly rounded one, and the rotation and
* the matrix of any q not 0, a unit q's own; the _fast forms are the former
* formulas, for a renderer's inner loops (multiply_fast, rotate_fast,
* to_matrix_fast, to_matrix4_fast, normalize_fast, slerp_fast).
*
* CONSTEXPR:
*   everything but from_axis_angle, slerp_fast and normalize_fast is a
* constant expression from C++14: the algebra and the rotations; norm,
* normalize, from_matrix, from_two_vectors and nlerp, on the C core's root
* in the form the level allows, the same bits either way; slerp, exp, log,
* sqrt and pow, the C core's accurate _c forms at every level.
* from_axis_angle and slerp_fast take the C library's sine and cosine at run
* time, and are constant expressions from C++20.
*
*
* path:      /inc/djinterp/cpp/modules/math/quaternion.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE QUATERNION
    --------------
2.  CONSTRUCTION
    ------------
3.  ALGEBRA
    -------
4.  ROTATION
    --------
5.  INTERPOLATION
    -------------
6.  EXPONENTIAL, LOGARITHM AND POWERS
    ---------------------------------
*/

#ifndef DJINTERP_MATH_QUATERNION_HPP
#define DJINTERP_MATH_QUATERNION_HPP 1

// std
#include <cstddef>                                 // std::size_t
// djinterp
#include "../../djinterp.hpp"                      // framework root
#include "./linear_algebra/linalg_common.hpp"      // internal::linalg_kernel
#include "./linear_algebra/matrix.hpp"             // linalg::matrix
#include "./linear_algebra/vector.hpp"             // linalg::vector


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE QUATERNION
//==============================================================================


// quaternion
//   struct: a quaternion x i + y j + z k + w, with vector part (x, y, z) and
// scalar part w. The default value is the identity rotation.
template<typename Type>
struct quaternion
{
    // ---- type aliases -------------------------------------------------------

    typedef Type                        value_type;
    typedef linalg::vector<Type, 3>     vector_type;

    // ---- data ---------------------------------------------------------------

    Type x;
    Type y;
    Type z;
    Type w;

    // ---- construction -------------------------------------------------------

    // default: the identity rotation (0, 0, 0, 1).
    D_CONSTEXPR
    quaternion() D_NOEXCEPT
        : x(static_cast<Type>(0)),
          y(static_cast<Type>(0)),
          z(static_cast<Type>(0)),
          w(static_cast<Type>(1))
    {}

    // from explicit components.
    D_CONSTEXPR
    quaternion(
        Type _x,
        Type _y,
        Type _z,
        Type _w
    ) D_NOEXCEPT
        : x(_x),
          y(_y),
          z(_z),
          w(_w)
    {}

    // ---- named factories ----------------------------------------------------

    // identity: the identity rotation.
    static D_CONSTEXPR quaternion
    identity() D_NOEXCEPT
    {
        return quaternion();
    }
};

NS_INTERNAL

    // quaternion_in, quaternion_out
    //   function: a quaternion as the C core's array [x, y, z, w], and back.
    template<typename Type>
    D_CONSTEXPR_CPP14 void
    quaternion_in(
        const quaternion<Type>& _q,
        Type*                   _out
    ) D_NOEXCEPT
    {
        _out[0] = _q.x;
        _out[1] = _q.y;
        _out[2] = _q.z;
        _out[3] = _q.w;
    }

    template<typename Type>
    D_CONSTEXPR_CPP14 quaternion<Type>
    quaternion_out(const Type* _q) D_NOEXCEPT
    {
        return quaternion<Type>(_q[0], _q[1], _q[2], _q[3]);
    }

NS_END  // internal


//==============================================================================
// 2.  CONSTRUCTION
//==============================================================================


// from_axis_angle
//   the rotation of _angle radians about _axis. The axis is normalized
// internally, so it need not be a unit vector (a zero one gives (0, 0, 0,
// cos(angle / 2))).
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<Type>
from_axis_angle(
    const linalg::vector<Type, 3>& _axis,
    Type                           _angle
) D_NOEXCEPT
{
    const linalg::vector<Type, 3> a    = _axis.normalized();
    Type                          q[4] = { Type(), Type(), Type(), Type() };

    linalg::internal::linalg_kernel<Type>::quat_from_unit_axis_angle(
        q, a.data(), _angle);

    return internal::quaternion_out(q);
}

// from_matrix
//   the unit quaternion, w >= 0, of the rotation matrix _m, by Shepperd's
// method: to_matrix's inverse, q or -q; of a 4x4 matrix, its upper-left
// 3x3. A matrix a few steps from orthogonal gives a quaternion as many from
// unit length.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
from_matrix(
    const linalg::matrix<Type, 3, 3>& _m
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    linalg::internal::linalg_kernel<Type>::quat_from_matrix3(q, &_m(0, 0));

    return internal::quaternion_out(q);
}

template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
from_matrix(
    const linalg::matrix<Type, 4, 4>& _m
) D_NOEXCEPT
{
    const Type m[9] = { _m(0, 0), _m(0, 1), _m(0, 2),
                        _m(1, 0), _m(1, 1), _m(1, 2),
                        _m(2, 0), _m(2, 1), _m(2, 2) };
    Type       q[4] = { Type(), Type(), Type(), Type() };

    linalg::internal::linalg_kernel<Type>::quat_from_matrix3(q, m);

    return internal::quaternion_out(q);
}

// from_two_vectors
//   the unit quaternion of the shortest rotation turning _a's direction into
// _b's: exactly opposed, a half turn about an axis perpendicular to _a; a
// zero vector, the identity.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
from_two_vectors(
    const linalg::vector<Type, 3>& _a,
    const linalg::vector<Type, 3>& _b
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    linalg::internal::linalg_kernel<Type>::quat_from_two_vectors(
        q, _a.data(), _b.data());

    return internal::quaternion_out(q);
}


//==============================================================================
// 3.  ALGEBRA
//==============================================================================


// operator*
//   the Hamilton product, composing the rotation _a after _b: each part
// within a step of the correctly rounded one, as accurate as twice the
// precision (S276).
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
operator*(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_multiply(a, a, b);

    return internal::quaternion_out(a);
}

// multiply_fast
//   the Hamilton product by the former formula, 16 products and 12 sums, for
// a renderer's inner loops.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
multiply_fast(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_multiply_fast(a, a, b);

    return internal::quaternion_out(a);
}

// inverse
//   1 / _q = q* / |q|^2; of 0, NaNs.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
inverse(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_inverse(q, q);

    return internal::quaternion_out(q);
}

// operator/
//   _a / _b = a b^-1, the quotient on the right (S277); inverse(b) * a is
// the left one, left_divide(b, a).
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
operator/(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_divide(a, a, b);

    return internal::quaternion_out(a);
}

// left_divide
//   _a \ _b = a^-1 b, the quotient on the left.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
left_divide(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_left_divide(a, a, b);

    return internal::quaternion_out(a);
}

// dot
//   the four-component inner product (the cosine of the angle between the two
// orientations on the unit hypersphere).
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 Type
dot(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);

    return linalg::internal::linalg_kernel<Type>::dot(a, b, 4);
}

// conjugate
//   the conjugate (x, y, z -> -x, -y, -z). For a unit quaternion this is the
// inverse rotation.
template<typename Type>
D_NODISCARD D_CONSTEXPR quaternion<Type>
conjugate(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    return quaternion<Type>(-_q.x, -_q.y, -_q.z, _q.w);
}

// norm
//   the Euclidean norm of the four components, within a step of the
// correctly rounded value, scaled so that it overflows only where it does.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 Type
norm(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);

    return linalg::internal::linalg_kernel<Type>::quat_norm(q);
}

// normalize
//   _q scaled to unit norm, each part within a step of q / |q|; a zero
// quaternion is returned as it is.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
normalize(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_normalize(q, q);

    return internal::quaternion_out(q);
}

// normalize_fast
//   _q times 1 / sqrt(x^2 + y^2 + z^2 + w^2), unscaled, for a renderer.
template<typename Type>
D_NODISCARD quaternion<Type>
normalize_fast(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_normalize_fast(q, q);

    return internal::quaternion_out(q);
}


//==============================================================================
// 4.  ROTATION
//==============================================================================


// rotate
//   _v rotated by _q, q v q^-1: any q not 0 names the rotation of q / |q|,
// each part within a step of the correctly rounded one.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
rotate(
    const quaternion<Type>&        _q,
    const linalg::vector<Type, 3>& _v
) D_NOEXCEPT
{
    Type                    q[4] = { Type(), Type(), Type(), Type() };
    linalg::vector<Type, 3> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_rotate(&r[0], q, _v.data());

    return r;
}

// rotate_fast
//   _v rotated by the unit quaternion _q by the former formula, v + w t + u
// x t, t = 2 u x v.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
rotate_fast(
    const quaternion<Type>&        _q,
    const linalg::vector<Type, 3>& _v
) D_NOEXCEPT
{
    Type                    q[4] = { Type(), Type(), Type(), Type() };
    linalg::vector<Type, 3> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_rotate_fast(&r[0], q,
                                                            _v.data());

    return r;
}

// to_matrix, to_matrix_fast
//   the 3x3 rotation matrix of _q: of any q not 0, each entry within a step
// of the correctly rounded one; and the former formula's, 1 - 2 (y^2 + z^2)
// and so on, of a unit q.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::matrix<Type, 3, 3>
to_matrix(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type                       q[4] = { Type(), Type(), Type(), Type() };
    linalg::matrix<Type, 3, 3> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_to_matrix3(&r(0, 0), q);

    return r;
}

template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::matrix<Type, 3, 3>
to_matrix_fast(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type                       q[4] = { Type(), Type(), Type(), Type() };
    linalg::matrix<Type, 3, 3> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_to_matrix3_fast(&r(0, 0), q);

    return r;
}

// to_matrix4, to_matrix4_fast
//   the 4x4 homogeneous rotation matrix: to_matrix's, or to_matrix_fast's,
// in its upper left.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::matrix<Type, 4, 4>
to_matrix4(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type                       q[4] = { Type(), Type(), Type(), Type() };
    linalg::matrix<Type, 4, 4> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_to_matrix4(&r(0, 0), q);

    return r;
}

template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 linalg::matrix<Type, 4, 4>
to_matrix4_fast(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type                       q[4] = { Type(), Type(), Type(), Type() };
    linalg::matrix<Type, 4, 4> r;

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_to_matrix4_fast(&r(0, 0), q);

    return r;
}


//==============================================================================
// 5.  INTERPOLATION
//==============================================================================


// slerp
//   spherical linear interpolation from _a (t = 0) to _b (t = 1) along the
// shorter arc at a constant speed, both unit quaternions: the angle 2
// atan2(|a - b|, |a + b|), accurate near 0; _a at t = 0 and _b or -_b at t
// = 1, exactly.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
slerp(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b,
    Type                    _t
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };
    Type r[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_slerp(r, a, b, _t);

    return internal::quaternion_out(r);
}

// slerp_fast
//   slerp by the same algorithm in Type's own precision on the C library's
// functions: a few steps of the largest part.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<Type>
slerp_fast(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b,
    Type                    _t
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };
    Type r[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_slerp_fast(r, a, b, _t);

    return internal::quaternion_out(r);
}

// nlerp
//   (1 - t) _a + t _b along the shorter arc, normalized: the cheap curve
// between two rotations, its speed varying along it.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
nlerp(
    const quaternion<Type>& _a,
    const quaternion<Type>& _b,
    Type                    _t
) D_NOEXCEPT
{
    Type a[4] = { Type(), Type(), Type(), Type() };
    Type b[4] = { Type(), Type(), Type(), Type() };
    Type r[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_a, a);
    internal::quaternion_in(_b, b);
    linalg::internal::linalg_kernel<Type>::quat_nlerp(r, a, b, _t);

    return internal::quaternion_out(r);
}


//==============================================================================
// 6.  EXPONENTIAL, LOGARITHM AND POWERS
//==============================================================================
// The complex functions in q's plane, principal values (S275): f(q) = Re f(w
// + r i) + u Im f(w + r i), r = |v|, u = v / r; for v along x alone, the
// complex function of w + x i itself. log(-1) is pi i.


// exp, log, sqrt
//   e^q; the principal logarithm, log |q| + u atan2(|v|, w); the principal
// square root, its real part >= 0.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
exp(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_exp(q, q);

    return internal::quaternion_out(q);
}

template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
log(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_log(q, q);

    return internal::quaternion_out(q);
}

template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
sqrt(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_sqrt(q, q);

    return internal::quaternion_out(q);
}

// pow
//   q^_p = exp(p log q) for a real exponent _p, the principal power.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 quaternion<Type>
pow(
    const quaternion<Type>& _q,
    Type                    _p
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);
    linalg::internal::linalg_kernel<Type>::quat_pow(q, q, _p);

    return internal::quaternion_out(q);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_QUATERNION_HPP
