/*******************************************************************************
* djinterp [math]                                                 quaternion.hpp
*
* Unit quaternions for representing and composing 3D rotations without gimbal
* lock.
*   quaternion<Type> stores the vector part (x, y, z) and scalar part w of the
* rotation x i + y j + z k + w. Construction is from an axis and angle; the
* Hamilton product composes rotations; a vector is rotated by the sandwich
* product; and the rotation converts to a 3x3 (or homogeneous 4x4) linalg
* matrix. Spherical linear interpolation (slerp) blends two orientations along
* the shortest arc. Rotation operations assume a unit quaternion.
*
* BRIDGE TO LINEAR ALGEBRA:
*   axes and rotated vectors are linalg::vector<Type, 3>; to_matrix /
* to_matrix4 yield linalg::matrix values, so a quaternion drops straight into
* the same transform pipeline as linalg::transform's rotation builders.
*
* THE C CORE:
*   a face over c/math/quat.h, as linear_algebra/quaternion.hpp is, through
* the same internal::linalg_kernel; the generic path serves any other
* element type. Everything compiles from C++98.
*
* CONSTEXPR:
*   the algebra -- the product, conjugate, dot, norm, normalize, rotate,
* to_matrix -- is a constant expression from C++14 (the norm on the C core's
* correctly rounded root). from_axis_angle and slerp need a sine and a
* cosine: the C library's at run time at every level, and constant
* expressions from C++20.
*
*
* path:      /inc/djinterp/math/quaternion.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE QUATERNION
    --------------
2.  CONSTRUCTION FROM AN AXIS AND ANGLE
    -----------------------------------
3.  ALGEBRA
    -------
4.  ROTATION
    --------
5.  INTERPOLATION
    -------------
*/

#ifndef DJINTERP_MATH_QUATERNION_HPP
#define DJINTERP_MATH_QUATERNION_HPP 1

// std
#include <cstddef>                                 // std::size_t
// djinterp
#include "../djinterp.hpp"                         // framework root
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
// 2.  CONSTRUCTION FROM AN AXIS AND ANGLE
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


//==============================================================================
// 3.  ALGEBRA
//==============================================================================


// operator*
//   the Hamilton product, composing the rotation _a after _b.
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
//   the Euclidean norm of the four components, correctly rounded.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP14 Type
norm(
    const quaternion<Type>& _q
) D_NOEXCEPT
{
    Type q[4] = { Type(), Type(), Type(), Type() };

    internal::quaternion_in(_q, q);

    return linalg::internal::linalg_kernel<Type>::norm(q, 4);
}

// normalize
//   _q scaled to unit norm; a zero quaternion is returned as it is.
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


//==============================================================================
// 4.  ROTATION
//==============================================================================


// rotate
//   _v rotated by the unit quaternion _q: v + w t + u x t, t = 2 u x v.
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

// to_matrix
//   the 3x3 rotation matrix of the unit quaternion _q.
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

// to_matrix4
//   the 4x4 homogeneous rotation matrix of the unit quaternion _q.
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


//==============================================================================
// 5.  INTERPOLATION
//==============================================================================


// slerp
//   spherical linear interpolation from _a (t = 0) to _b (t = 1) along the
// shorter arc; both are unit quaternions.
template<typename Type>
D_NODISCARD D_CONSTEXPR_CPP20 quaternion<Type>
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


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_QUATERNION_HPP
