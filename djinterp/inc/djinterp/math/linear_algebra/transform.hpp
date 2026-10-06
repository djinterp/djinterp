/*******************************************************************************
* djinterp [math]                                                  transform.hpp
*
* Transformation builders and homogeneous coordinates for the linear-algebra
* subframework.
*   Free function templates that construct the standard linear and affine
* transformation matrices (scaling, rotation, translation), the homogeneous-
* coordinate machinery that ties them together, and the bridge to the geometry
* subframework. Every routine returns a core vector / matrix value, so results
* compose with the fluent members and with the products in matrix.hpp:
*   T = homogeneous(rotation_z(angle), offset);  // 4x4 affine
*   p = transform_point(T, q);
*
* PROVIDED FUNCTIONS:
*   linear builders
*     scaling_2d(sx, sy) / scaling_2d(s)            -> 2x2
*     scaling_3d(sx, sy, sz) / scaling_3d(s)        -> 3x3
*     rotation_2d(angle)                            -> 2x2
*     rotation_x / rotation_y / rotation_z(angle)   -> 3x3
*     rotation_axis(axis, angle)                    -> 3x3 (Rodrigues)
*   homogeneous embedding
*     homogeneous(linear)                           -> (N+1)x(N+1)
*     homogeneous(linear, translation)              -> (N+1)x(N+1)
*     translation(t) / translation_2d / translation_3d
*   homogeneous coordinates
*     to_homogeneous(v) / from_homogeneous(h)
*     transform_point(M, p)                         -- affine/projective point
*     transform_direction(M, d)                     -- linear part only
*   geometry bridge (C++11, where std::array exists)
*     to_array(v)    -- linalg vector  -> std::array (a geometry point_type)
*     to_vector(a)   -- std::array     -> linalg vector
*
* DESIGN NOTES:
*   - A face over the C core: the rotations are c/math/mat.h's kernels, from
*     the angle's cosine and sine, which are the C library's at run time at
*     every level and constant expressions from C++20 (elementary.h); the
*     rest builds on the matrix and vector faces. Everything compiles from
*     C++98; what takes no angle is a constant expression from C++14.
*   - Rotations and the projective divide require a floating-point element
*     type; scaling, translation, the homogeneous embedding, to_homogeneous,
*     transform_direction, and the bridge are generic over arithmetic types.
*   - Homogeneous transforms are (N+1)x(N+1) and act on (N+1)-vectors. An N-point
*     embeds as (p, 1); from_homogeneous performs the perspective divide by the
*     final component. A direction embeds as (d, 0), so translation does not
*     affect it.
*   - GEOMETRY BRIDGE: the geometry subframework represents a point as
*     std::array<value_type, N> (cartesian::point_type, point_2d, point_3d).
*     to_array / to_vector convert directly between that representation and a
*     linalg vector, so a transform built here applies to geometry points
*     without any coupling between the two subframeworks' headers.
*
*
* path:      /inc/djinterp/math/linear_algebra/transform.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SCALING
    -------
2.  ROTATION
    --------
3.  HOMOGENEOUS EMBEDDING
    ---------------------
4.  HOMOGENEOUS COORDINATES
    -----------------------
5.  GEOMETRY BRIDGE
    ---------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_TRANSFORM_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_TRANSFORM_HPP 1

// std
#include <cstddef>                 // std::size_t
// djinterp
#include "../../djinterp.hpp"      // framework root
#include "./linalg_common.hpp"     // internal::linalg_kernel,
                                   // internal::linalg_elementary
#include "./matrix.hpp"            // matrix
#include "./vector.hpp"            // vector
// re_std
#include "../../../re_std/type_traits/is_floating_point.hpp"  // is_floating_
                                                              // point
#include "../../../re_std/utility/integer_sequence.hpp"       // index_
                                                              // sequence
#include "../../../re_std/utility/make_integer_sequence.hpp"  // make_index_
                                                              // sequence
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>               // std::array
#endif


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  SCALING
//==============================================================================


// scaling_2d
//   function: a 2x2 non-uniform scaling matrix.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 2, 2>
scaling_2d(
    Type _sx,
    Type _sy
) D_NOEXCEPT
{
    return matrix<Type, 2, 2>::diagonal(vector<Type, 2>(_sx, _sy));
}

// scaling_2d
//   function: a 2x2 uniform scaling matrix.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 2, 2>
scaling_2d(Type _s) D_NOEXCEPT
{
    return matrix<Type, 2, 2>::diagonal(vector<Type, 2>(_s, _s));
}

// scaling_3d
//   function: a 3x3 non-uniform scaling matrix.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 3, 3>
scaling_3d(
    Type _sx,
    Type _sy,
    Type _sz
) D_NOEXCEPT
{
    return matrix<Type, 3, 3>::diagonal(vector<Type, 3>(_sx, _sy, _sz));
}

// scaling_3d
//   function: a 3x3 uniform scaling matrix.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 3, 3>
scaling_3d(Type _s) D_NOEXCEPT
{
    return matrix<Type, 3, 3>::diagonal(vector<Type, 3>(_s, _s, _s));
}


//==============================================================================
// 2.  ROTATION
//==============================================================================
// Counter-clockwise by an angle in radians; floating-point element types
// only. The cosine and sine are the C library's at run time at every level
// and constant expressions from C++20, in the element type for float,
// double and long double and through double for any other.


NS_INTERNAL

    // linalg_cos_sin
    //   struct: an angle's cosine and sine, in Type.
    template<typename Type>
    struct linalg_cos_sin
    {
        typedef typename linalg_kernel<Type>::root_type root_type;

        Type c;
        Type s;

        D_CONSTEXPR_CPP20
        linalg_cos_sin(Type _angle) D_NOEXCEPT
            : c(static_cast<Type>(linalg_elementary<root_type>::cos(
                  static_cast<root_type>(_angle)))),
              s(static_cast<Type>(linalg_elementary<root_type>::sin(
                  static_cast<root_type>(_angle))))
        {}
    };

NS_END  // internal

// rotation_2d
//   function: a 2x2 counter-clockwise rotation by _angle radians.
template<typename Type>
D_CONSTEXPR_CPP20 matrix<Type, 2, 2>
rotation_2d(Type _angle) D_NOEXCEPT
{
    // rotation_2d: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    const internal::linalg_cos_sin<Type> cs(_angle);
    matrix<Type, 2, 2>                   m;

    internal::linalg_kernel<Type>::rotation_2d(&m(0, 0), cs.c, cs.s);

    return m;
}

// rotation_x
//   function: a 3x3 rotation about the x axis by _angle radians.
template<typename Type>
D_CONSTEXPR_CPP20 matrix<Type, 3, 3>
rotation_x(Type _angle) D_NOEXCEPT
{
    // rotation_x: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    const internal::linalg_cos_sin<Type> cs(_angle);
    matrix<Type, 3, 3>                   m;

    internal::linalg_kernel<Type>::rotation_3d(&m(0, 0), 0, cs.c, cs.s);

    return m;
}

// rotation_y
//   function: a 3x3 rotation about the y axis by _angle radians.
template<typename Type>
D_CONSTEXPR_CPP20 matrix<Type, 3, 3>
rotation_y(Type _angle) D_NOEXCEPT
{
    // rotation_y: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    const internal::linalg_cos_sin<Type> cs(_angle);
    matrix<Type, 3, 3>                   m;

    internal::linalg_kernel<Type>::rotation_3d(&m(0, 0), 1, cs.c, cs.s);

    return m;
}

// rotation_z
//   function: a 3x3 rotation about the z axis by _angle radians.
template<typename Type>
D_CONSTEXPR_CPP20 matrix<Type, 3, 3>
rotation_z(Type _angle) D_NOEXCEPT
{
    // rotation_z: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    const internal::linalg_cos_sin<Type> cs(_angle);
    matrix<Type, 3, 3>                   m;

    internal::linalg_kernel<Type>::rotation_3d(&m(0, 0), 2, cs.c, cs.s);

    return m;
}

// rotation_axis
//   function: a 3x3 rotation by _angle radians about _axis (normalized
// here), by Rodrigues's formula.
template<typename Type>
D_CONSTEXPR_CPP20 matrix<Type, 3, 3>
rotation_axis(
    const vector<Type, 3>& _axis,
    Type                   _angle
) D_NOEXCEPT
{
    // rotation_axis: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    const vector<Type, 3>                k = _axis.normalized();
    const internal::linalg_cos_sin<Type> cs(_angle);
    matrix<Type, 3, 3>                   m;

    internal::linalg_kernel<Type>::rotation_axis(&m(0, 0),
                                                 k.data(),
                                                 cs.c,
                                                 cs.s);

    return m;
}


//==============================================================================
// 3.  HOMOGENEOUS EMBEDDING
//==============================================================================


// homogeneous
//   function: an N x N linear transform as an (N+1) x (N+1) homogeneous
// transform with zero translation.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N + 1, N + 1>
homogeneous(const matrix<Type, N, N>& _linear) D_NOEXCEPT
{
    matrix<Type, N + 1, N + 1> m = matrix<Type, N + 1, N + 1>::identity();

    for (std::size_t i = 0; i < N; ++i)
    {
        for (std::size_t j = 0; j < N; ++j)
        {
            m(i, j) = _linear(i, j);
        }
    }

    return m;
}

// homogeneous
//   function: an N x N linear transform together with an N-vector
// translation as an (N+1) x (N+1) homogeneous transform.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N + 1, N + 1>
homogeneous(
    const matrix<Type, N, N>& _linear,
    const vector<Type, N>&    _translation
) D_NOEXCEPT
{
    matrix<Type, N + 1, N + 1> m = homogeneous(_linear);

    for (std::size_t i = 0; i < N; ++i)
    {
        m(i, N) = _translation[i];
    }

    return m;
}

// translation
//   function: an (N+1) x (N+1) homogeneous pure-translation transform.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N + 1, N + 1>
translation(const vector<Type, N>& _t) D_NOEXCEPT
{
    matrix<Type, N + 1, N + 1> m = matrix<Type, N + 1, N + 1>::identity();

    for (std::size_t i = 0; i < N; ++i)
    {
        m(i, N) = _t[i];
    }

    return m;
}

// translation_2d
//   function: a 3x3 homogeneous translation.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 3, 3>
translation_2d(
    Type _tx,
    Type _ty
) D_NOEXCEPT
{
    return translation(vector<Type, 2>(_tx, _ty));
}

// translation_3d
//   function: a 4x4 homogeneous translation.
template<typename Type>
D_CONSTEXPR_CPP14 matrix<Type, 4, 4>
translation_3d(
    Type _tx,
    Type _ty,
    Type _tz
) D_NOEXCEPT
{
    return translation(vector<Type, 3>(_tx, _ty, _tz));
}


//==============================================================================
// 4.  HOMOGENEOUS COORDINATES
//==============================================================================


// to_homogeneous
//   function: an N-vector lifted to its (N+1)-dimensional homogeneous form
// (w = 1).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N + 1>
to_homogeneous(const vector<Type, N>& _v) D_NOEXCEPT
{
    vector<Type, N + 1> h;

    for (std::size_t i = 0; i < N; ++i)
    {
        h[i] = _v[i];
    }

    h[N] = static_cast<Type>(1);

    return h;
}

// from_homogeneous
//   function: an M-vector projected back to M-1 dimensions, divided by its
// final component (the perspective divide), or left alone when it is zero.
template<typename    Type,
         std::size_t M>
D_CONSTEXPR_CPP14 vector<Type, M - 1>
from_homogeneous(const vector<Type, M>& _h) D_NOEXCEPT
{
    // from_homogeneous: requires a floating-point element type and
    // dimension >= 2
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();
    (void)internal::linalg_requires<(M >= 2)>::check();

    const Type          w = _h[M - 1];
    vector<Type, M - 1> v;

    // the divide, or the components as they are when w is zero
    if (w != static_cast<Type>(0))
    {
        internal::linalg_kernel<Type>::divide(&v[0], _h.data(), w, M - 1);
    }
    else
    {
        for (std::size_t i = 0; i < (M - 1); ++i)
        {
            v[i] = _h[i];
        }
    }

    return v;
}

// transform_point
//   function: an (N+1) x (N+1) homogeneous transform applied to an N-point:
// embedded as (p, 1), multiplied, then perspective-divided.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
transform_point(
    const matrix<Type, N + 1, N + 1>& _m,
    const vector<Type, N>&            _p
) D_NOEXCEPT
{
    // transform_point: requires a floating-point element type
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    return from_homogeneous(_m.times(to_homogeneous(_p)));
}

// transform_direction
//   function: only the linear part of an (N+1) x (N+1) homogeneous
// transform applied to an N-direction (embedded as (d, 0), so translation
// has no effect).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
transform_direction(
    const matrix<Type, N + 1, N + 1>& _m,
    const vector<Type, N>&            _d
) D_NOEXCEPT
{
    vector<Type, N + 1> hd;

    for (std::size_t i = 0; i < N; ++i)
    {
        hd[i] = _d[i];
    }

    const vector<Type, N + 1> e = _m.times(hd);
    vector<Type, N>           r;

    for (std::size_t i = 0; i < N; ++i)
    {
        r[i] = e[i];
    }

    return r;
}


//==============================================================================
// 5.  GEOMETRY BRIDGE
//==============================================================================
// From C++11, where std::array exists; to_array is a constant expression
// from C++11, by pack expansion into an aggregate initialization.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
NS_INTERNAL

    // to_array_helper
    //   function: a std::array from a vector, by pack expansion.
    template<typename    Type,
             std::size_t N,
             std::size_t... Is>
    D_CONSTEXPR std::array<Type, N>
    to_array_helper(
        const vector<Type, N>& _v,
        re_std::index_sequence<Is...>
    ) noexcept
    {
        return std::array<Type, N>{ { _v[Is]... } };
    }

NS_END  // internal

// to_array
//   function: a linalg vector as a std::array -- the geometry
// subframework's point_type (cartesian::point_type, point_2d, point_3d).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR std::array<Type, N>
to_array(const vector<Type, N>& _v) noexcept
{
    return internal::to_array_helper(_v, re_std::make_index_sequence<N>());
}

// to_vector
//   function: a std::array (a geometry point_type) as a linalg vector.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<Type, N>
to_vector(const std::array<Type, N>& _a) noexcept
{
    return vector<Type, N>::from_array(_a);
}
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_TRANSFORM_HPP
