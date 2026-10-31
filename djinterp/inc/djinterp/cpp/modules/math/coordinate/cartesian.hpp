/*******************************************************************************
* djinterp [cpp/math]                                              cartesian.hpp
*
* Compile-time N-dimensional Cartesian coordinate system.
*   Provides the Cartesian coordinate system type, point representation,
* scale factors, and differential geometry infrastructure. Cartesian
* coordinates are the simplest orthogonal system: all scale factors are 1,
* the Jacobian is 1, and the basis vectors are the standard unit vectors.
*
* STRUCTURAL INTERFACE (for coordinate traits):
*   - value_type, point_type
*   - static dimension
*   - static bool is_cartesian  = true
*   - static bool is_polar      = false
*   - static bool is_cylindrical = false
*   - static bool is_spherical  = false
*   - static bool is_orthogonal = true
*   - scale_factors(point), jacobian(point)
*   - to_cartesian(point), from_cartesian(point)
*
* AXIS NAMING:
*   1D: x
*   2D: x, y
*   3D: x, y, z
*   ND: x₀, x₁, ..., x_{N-1}
*
* A FACE OVER THE C CORE:
*   A point is a linalg::vector<value_type, Dimension>, written {{x, y, z}};
* its distances, norms and products are the vector's, so c/math/vec.h's.
* Everything compiles from C++98; what takes no root is a constant
* expression from C++14, and the roots too as the vector's are.
*
*
* path:      /inc/djinterp/cpp/modules/math/coordinate/cartesian.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE CARTESIAN COORDINATE SYSTEM
    -------------------------------
    1.  cartesian
2.  TYPE ALIASES
    ------------
*/

#ifndef DJINTERP_MATH_COORDINATE_CARTESIAN_HPP
#define DJINTERP_MATH_COORDINATE_CARTESIAN_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../linear_algebra/vector.hpp"           // linalg::vector


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE CARTESIAN COORDINATE SYSTEM
//==============================================================================


// 1.1    cartesian
//------------------------------------------------------------------------------
// 1.1.1
// cartesian
//   struct: N-dimensional Cartesian coordinate system, the identity of the
// coordinate maps: every scale factor and the Jacobian are 1.
template<std::size_t Dimension = 3,
         typename    ValueType = double>
struct cartesian
{
    // ---- type aliases -------------------------------------------------------

    typedef ValueType                                  value_type;
    typedef linalg::vector<value_type, Dimension>      point_type;
    typedef linalg::vector<value_type, Dimension>      vector_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension      = Dimension;
    static D_CONSTEXPR_VAR bool        is_cartesian   = true;
    static D_CONSTEXPR_VAR bool        is_polar       = false;
    static D_CONSTEXPR_VAR bool        is_cylindrical = false;
    static D_CONSTEXPR_VAR bool        is_spherical   = false;
    static D_CONSTEXPR_VAR bool        is_orthogonal  = true;

    D_STATIC_ASSERT((Dimension > 0),
                    "cartesian: dimension must be at least 1.");

    // ---- coordinate access --------------------------------------------------

    // get
    //   the I-th coordinate.
    template<std::size_t I>
    static D_CONSTEXPR_CPP14 value_type
    get(const point_type& _point) D_NOEXCEPT
    {
        D_STATIC_ASSERT((I < Dimension),
                        "cartesian::get: the index must be below the "
                        "dimension.");

        return _point[I];
    }

    // x, y, z
    //   the first three coordinates (y from 2 dimensions, z from 3).
    static D_CONSTEXPR value_type
    x(const point_type& _point) D_NOEXCEPT
    {
        return _point[0];
    }

    static D_CONSTEXPR_CPP14 value_type
    y(const point_type& _point) D_NOEXCEPT
    {
        D_STATIC_ASSERT((Dimension >= 2),
                        "cartesian::y: requires dimension >= 2.");

        return _point[1];
    }

    static D_CONSTEXPR_CPP14 value_type
    z(const point_type& _point) D_NOEXCEPT
    {
        D_STATIC_ASSERT((Dimension >= 3),
                        "cartesian::z: requires dimension >= 3.");

        return _point[2];
    }

    // ---- point construction -------------------------------------------------

    // origin
    //   the origin (all zeros).
    static D_CONSTEXPR point_type
    origin() D_NOEXCEPT
    {
        return point_type();
    }

    // unit
    //   the I-th standard basis vector.
    template<std::size_t I>
    static D_CONSTEXPR_CPP14 point_type
    unit() D_NOEXCEPT
    {
        return point_type::template basis<I>();
    }

    // make_point
    //   a point from its coordinates (two for a 2D system, three for 3D).
    static D_CONSTEXPR_CPP14 point_type
    make_point(
        value_type _x,
        value_type _y
    ) D_NOEXCEPT
    {
        D_STATIC_ASSERT((Dimension == 2),
                        "cartesian::make_point(x, y): only for a 2D system.");

        const value_type parts[2] = { _x, _y };

        return point_type(parts);
    }

    static D_CONSTEXPR_CPP14 point_type
    make_point(
        value_type _x,
        value_type _y,
        value_type _z
    ) D_NOEXCEPT
    {
        D_STATIC_ASSERT((Dimension == 3),
                        "cartesian::make_point(x, y, z): only for a 3D "
                        "system.");

        const value_type parts[3] = { _x, _y, _z };

        return point_type(parts);
    }

    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   every scale factor, 1.
    static D_CONSTEXPR_CPP14 vector_type
    scale_factors(const point_type&) D_NOEXCEPT
    {
        return vector_type::filled(static_cast<value_type>(1));
    }

    // jacobian, volume_element
    //   1.
    static D_CONSTEXPR value_type
    jacobian(const point_type&) D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    static D_CONSTEXPR value_type
    volume_element(const point_type&) D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // ---- coordinate conversion ----------------------------------------------

    // to_cartesian, from_cartesian
    //   the point itself.
    static D_CONSTEXPR point_type
    to_cartesian(const point_type& _point) D_NOEXCEPT
    {
        return _point;
    }

    static D_CONSTEXPR point_type
    from_cartesian(const point_type& _point) D_NOEXCEPT
    {
        return _point;
    }

    // ---- metric -------------------------------------------------------------

    // distance_squared, distance, norm_squared, norm, dot
    //   the vector's.
    static D_CONSTEXPR_CPP14 value_type
    distance_squared(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return _a.distance_squared(_b);
    }

    static D_CONSTEXPR_CPP14 value_type
    distance(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return _a.distance(_b);
    }

    static D_CONSTEXPR_CPP14 value_type
    norm_squared(const point_type& _point) D_NOEXCEPT
    {
        return _point.norm_squared();
    }

    static D_CONSTEXPR_CPP14 value_type
    norm(const point_type& _point) D_NOEXCEPT
    {
        return _point.norm();
    }

    static D_CONSTEXPR_CPP14 value_type
    dot(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return _a.dot(_b);
    }

    // cross
    //   the cross product (3D systems).
    static D_CONSTEXPR_CPP14 point_type
    cross(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return _a.cross(_b);
    }
};

// the static constants' definitions (a C++98 program that takes one's
// address needs one; from C++17 the in-class declarations are definitions)
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR std::size_t cartesian<Dimension, ValueType>::dimension;
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR bool cartesian<Dimension, ValueType>::is_cartesian;
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR bool cartesian<Dimension, ValueType>::is_polar;
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR bool cartesian<Dimension, ValueType>::is_cylindrical;
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR bool cartesian<Dimension, ValueType>::is_spherical;
template<std::size_t Dimension, typename ValueType>
D_CONSTEXPR_VAR bool cartesian<Dimension, ValueType>::is_orthogonal;


//==============================================================================
// 2.  TYPE ALIASES
//==============================================================================
// The alias templates are C++11's; below it, cartesian<N, T> is the name.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename ValueType = double>
using cartesian_1d = cartesian<1, ValueType>;
template<typename ValueType = double>
using cartesian_2d = cartesian<2, ValueType>;
template<typename ValueType = double>
using cartesian_3d = cartesian<3, ValueType>;
template<typename ValueType = double>
using cartesian_4d = cartesian<4, ValueType>;
template<std::size_t N, typename ValueType = double>
using cartesian_nd = cartesian<N, ValueType>;
template<std::size_t N, typename ValueType = double>
using cartesian_point = typename cartesian<N, ValueType>::point_type;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_CARTESIAN_HPP
