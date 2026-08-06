/******************************************************************************
* djinterp [math]                                                transform.hpp
*
* Transformation builders and homogeneous coordinates for the linear-algebra
* subframework.
*   Free function templates that construct the standard linear and affine
* transformation matrices (scaling, rotation, translation), the homogeneous-
* coordinate machinery that ties them together, and the bridge to the geometry
* subframework. Every routine returns a core vector / matrix value, so results
* compose with the fluent members and with the products in matrix.hpp:
*   auto T = homogeneous(rotation_z(angle), offset);   // 4x4 affine
*   auto p = transform_point(T, q);
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
*   geometry bridge
*     to_array(v)    -- linalg vector  -> std::array (a geometry point_type)
*     to_vector(a)   -- std::array     -> linalg vector
*
* DESIGN NOTES:
*   - Rotations and the projective divide require a floating-point element type;
*     scaling, translation, the homogeneous embedding, to_homogeneous,
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
* path:      /inc/djinterp/math/linear_algebra/transform.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_TRANSFORM_
#define DJINTERP_MATH_LINALG_TRANSFORM_ 1

// std
#include <cstddef>
#include <array>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./linalg_common.hpp"
#include "./vector.hpp"
#include "./matrix.hpp"


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   SCALING  (linear)
// ===========================================================================

// scaling_2d
//   a 2x2 non-uniform scaling matrix.
template<typename _T>
D_CONSTEXPR matrix<_T, 2, 2>
scaling_2d(
    _T _sx,
    _T _sy
) noexcept
{
    return matrix<_T, 2, 2>::diagonal(vector<_T, 2>(_sx, _sy));
}

// scaling_2d
//   a 2x2 uniform scaling matrix.
template<typename _T>
D_CONSTEXPR matrix<_T, 2, 2>
scaling_2d(_T _s) noexcept
{
    return matrix<_T, 2, 2>::diagonal(vector<_T, 2>(_s, _s));
}

// scaling_3d
//   a 3x3 non-uniform scaling matrix.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
scaling_3d(
    _T _sx,
    _T _sy,
    _T _sz
) noexcept
{
    return matrix<_T, 3, 3>::diagonal(vector<_T, 3>(_sx, _sy, _sz));
}

// scaling_3d
//   a 3x3 uniform scaling matrix.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
scaling_3d(_T _s) noexcept
{
    return matrix<_T, 3, 3>::diagonal(vector<_T, 3>(_s, _s, _s));
}


// ===========================================================================
// II.  ROTATION  (linear)
// ===========================================================================

// rotation_2d
//   a 2x2 counter-clockwise rotation by _angle radians.
template<typename _T>
D_CONSTEXPR matrix<_T, 2, 2>
rotation_2d(_T _angle) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "rotation_2d: requires a floating-point element type.");

    const _T c = static_cast<_T>(internal::cos_c(static_cast<double>(_angle)));
    const _T s = static_cast<_T>(internal::sin_c(static_cast<double>(_angle)));

    matrix<_T, 2, 2> m;

    m(0, 0) = c;
    m(0, 1) = -s;
    m(1, 0) = s;
    m(1, 1) = c;

    return m;
}

// rotation_x
//   a 3x3 rotation by _angle radians about the x-axis.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
rotation_x(_T _angle) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "rotation_x: requires a floating-point element type.");

    const _T c = static_cast<_T>(internal::cos_c(static_cast<double>(_angle)));
    const _T s = static_cast<_T>(internal::sin_c(static_cast<double>(_angle)));

    matrix<_T, 3, 3> m = matrix<_T, 3, 3>::identity();

    m(1, 1) = c;
    m(1, 2) = -s;
    m(2, 1) = s;
    m(2, 2) = c;

    return m;
}

// rotation_y
//   a 3x3 rotation by _angle radians about the y-axis.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
rotation_y(_T _angle) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "rotation_y: requires a floating-point element type.");

    const _T c = static_cast<_T>(internal::cos_c(static_cast<double>(_angle)));
    const _T s = static_cast<_T>(internal::sin_c(static_cast<double>(_angle)));

    matrix<_T, 3, 3> m = matrix<_T, 3, 3>::identity();

    m(0, 0) = c;
    m(0, 2) = s;
    m(2, 0) = -s;
    m(2, 2) = c;

    return m;
}

// rotation_z
//   a 3x3 rotation by _angle radians about the z-axis.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
rotation_z(_T _angle) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "rotation_z: requires a floating-point element type.");

    const _T c = static_cast<_T>(internal::cos_c(static_cast<double>(_angle)));
    const _T s = static_cast<_T>(internal::sin_c(static_cast<double>(_angle)));

    matrix<_T, 3, 3> m = matrix<_T, 3, 3>::identity();

    m(0, 0) = c;
    m(0, 1) = -s;
    m(1, 0) = s;
    m(1, 1) = c;

    return m;
}

// rotation_axis
//   a 3x3 rotation by _angle radians about an arbitrary axis (Rodrigues'
// rotation formula). The axis is normalized internally.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
rotation_axis(
    const vector<_T, 3>& _axis,
    _T                   _angle
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "rotation_axis: requires a floating-point element type.");

    const vector<_T, 3> k = _axis.normalized();

    const _T c = static_cast<_T>(internal::cos_c(static_cast<double>(_angle)));
    const _T s = static_cast<_T>(internal::sin_c(static_cast<double>(_angle)));
    const _T t = static_cast<_T>(1) - c;

    const _T kx = k[0];
    const _T ky = k[1];
    const _T kz = k[2];

    matrix<_T, 3, 3> m;

    m(0, 0) = c + kx * kx * t;
    m(0, 1) = kx * ky * t - kz * s;
    m(0, 2) = kx * kz * t + ky * s;

    m(1, 0) = ky * kx * t + kz * s;
    m(1, 1) = c + ky * ky * t;
    m(1, 2) = ky * kz * t - kx * s;

    m(2, 0) = kz * kx * t - ky * s;
    m(2, 1) = kz * ky * t + kx * s;
    m(2, 2) = c + kz * kz * t;

    return m;
}


// ===========================================================================
// III. HOMOGENEOUS EMBEDDING
// ===========================================================================

// homogeneous
//   embed an N x N linear transform as an (N+1) x (N+1) homogeneous transform
// with zero translation.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N + 1, _N + 1>
homogeneous(const matrix<_T, _N, _N>& _linear) noexcept
{
    matrix<_T, _N + 1, _N + 1> m = matrix<_T, _N + 1, _N + 1>::identity();

    for (std::size_t i = 0; i < _N; ++i)
    {
        for (std::size_t j = 0; j < _N; ++j)
        {
            m(i, j) = _linear(i, j);
        }
    }

    return m;
}

// homogeneous
//   embed an N x N linear transform together with an N-vector translation as an
// (N+1) x (N+1) homogeneous transform.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N + 1, _N + 1>
homogeneous(
    const matrix<_T, _N, _N>& _linear,
    const vector<_T, _N>&     _translation
) noexcept
{
    matrix<_T, _N + 1, _N + 1> m = matrix<_T, _N + 1, _N + 1>::identity();

    for (std::size_t i = 0; i < _N; ++i)
    {
        for (std::size_t j = 0; j < _N; ++j)
        {
            m(i, j) = _linear(i, j);
        }

        m(i, _N) = _translation[i];
    }

    return m;
}

// translation
//   an (N+1) x (N+1) homogeneous pure-translation transform.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N + 1, _N + 1>
translation(const vector<_T, _N>& _t) noexcept
{
    matrix<_T, _N + 1, _N + 1> m = matrix<_T, _N + 1, _N + 1>::identity();

    for (std::size_t i = 0; i < _N; ++i)
    {
        m(i, _N) = _t[i];
    }

    return m;
}

// translation_2d
//   a 3x3 homogeneous translation.
template<typename _T>
D_CONSTEXPR matrix<_T, 3, 3>
translation_2d(
    _T _tx,
    _T _ty
) noexcept
{
    return translation(vector<_T, 2>(_tx, _ty));
}

// translation_3d
//   a 4x4 homogeneous translation.
template<typename _T>
D_CONSTEXPR matrix<_T, 4, 4>
translation_3d(
    _T _tx,
    _T _ty,
    _T _tz
) noexcept
{
    return translation(vector<_T, 3>(_tx, _ty, _tz));
}


// ===========================================================================
// IV.  HOMOGENEOUS COORDINATES
// ===========================================================================

// to_homogeneous
//   lift an N-vector to its (N+1)-dimensional homogeneous form (w = 1).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N + 1>
to_homogeneous(const vector<_T, _N>& _v) noexcept
{
    vector<_T, _N + 1> h;

    for (std::size_t i = 0; i < _N; ++i)
    {
        h[i] = _v[i];
    }

    h[_N] = static_cast<_T>(1);

    return h;
}

// from_homogeneous
//   project an M-vector back to M-1 dimensions, dividing by the final
// component (the perspective divide).
template<typename    _T,
         std::size_t _M>
D_CONSTEXPR vector<_T, _M - 1>
from_homogeneous(const vector<_T, _M>& _h) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "from_homogeneous: requires a floating-point element type.");
    static_assert((_M >= 2),
                  "from_homogeneous: requires dimension >= 2.");

    const _T w = _h[_M - 1];

    vector<_T, _M - 1> v;

    for (std::size_t i = 0; i < (_M - 1); ++i)
    {
        v[i] = (w != static_cast<_T>(0)) ? (_h[i] / w) : _h[i];
    }

    return v;
}

// transform_point
//   apply an (N+1) x (N+1) homogeneous transform to an N-point (embed as
// (p, 1), multiply, then perspective-divide).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
transform_point(
    const matrix<_T, _N + 1, _N + 1>& _m,
    const vector<_T, _N>&             _p
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "transform_point: requires a floating-point element type.");

    return from_homogeneous(_m.times(to_homogeneous(_p)));
}

// transform_direction
//   apply only the linear part of an (N+1) x (N+1) homogeneous transform to an
// N-direction (embed as (d, 0), so translation has no effect).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
transform_direction(
    const matrix<_T, _N + 1, _N + 1>& _m,
    const vector<_T, _N>&             _d
) noexcept
{
    vector<_T, _N + 1> hd;

    for (std::size_t i = 0; i < _N; ++i)
    {
        hd[i] = _d[i];
    }

    const vector<_T, _N + 1> e = _m.times(hd);

    vector<_T, _N> r;

    for (std::size_t i = 0; i < _N; ++i)
    {
        r[i] = e[i];
    }

    return r;
}


// ===========================================================================
// V.   GEOMETRY BRIDGE
// ===========================================================================

NS_INTERNAL

    // to_array_helper
    //   build a std::array from a vector via pack expansion (an aggregate
    // initialization, so it stays constexpr without per-element mutation).
    template<typename    _T,
             std::size_t _N,
             std::size_t... _Is>
    D_CONSTEXPR std::array<_T, _N>
    to_array_helper(
        const vector<_T, _N>&             _v,
        std::index_sequence<_Is...>
    ) noexcept
    {
        return std::array<_T, _N>{ { _v[_Is]... } };
    }

NS_END  // internal

// to_array
//   convert a linalg vector to a std::array -- the geometry subframework's
// point_type (cartesian::point_type, point_2d, point_3d).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR std::array<_T, _N>
to_array(const vector<_T, _N>& _v) noexcept
{
    return internal::to_array_helper(_v, std::make_index_sequence<_N>{});
}

// to_vector
//   convert a std::array (a geometry point_type) to a linalg vector.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
to_vector(const std::array<_T, _N>& _a) noexcept
{
    return vector<_T, _N>::from_array(_a);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_TRANSFORM_
