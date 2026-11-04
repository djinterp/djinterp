/*******************************************************************************
* djinterp [cpp/math]                                                  polar.hpp
*
* Compile-time 2D polar coordinate system (r, θ).
*   Provides the polar coordinate system type, point representation,
* scale factors, differential geometry infrastructure, and conversion
* to/from Cartesian coordinates.
*
* COORDINATES:
*   r     - radial distance from origin       [0, ∞)
*   θ     - angle from positive x-axis        [0, 2π) or (-π, π]
*
* SCALE FACTORS:
*   h_r = 1,  h_θ = r
*
* LINE ELEMENT:
*   ds² = dr² + r² dθ²
*
* AREA ELEMENT:
*   dA = r dr dθ
*
* CONVERSION:
*   x = r cos θ,  y = r sin θ
*   r = √(x² + y²),  θ = atan2(y, x)
*
* A FACE OVER THE C CORE:
*   The conversions, the distance and the canonical forms are
* c/math/coord.h's, through its _auto names: the C library's sine, cosine,
* root and arc-tangent at run time at every level, constant expressions
* from C++20, and through double for an element type other than float,
* double and long double. A point is a linalg::vector<value_type, 2>,
* written {{r, theta}}. Everything compiles from C++98.
*
*
* path:      /inc/djinterp/cpp/modules/math/coordinate/polar.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE POLAR COORDINATE SYSTEM
    ---------------------------
    1.  polar
2.  TYPE ALIASES
    ------------
*/

#ifndef DJINTERP_MATH_COORDINATE_POLAR_HPP
#define DJINTERP_MATH_COORDINATE_POLAR_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../../c/modules/math/coord.h"     // the C core: d_coord_*
#include "../linear_algebra/vector.hpp"           // linalg::vector


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE POLAR COORDINATE SYSTEM
//==============================================================================


// 1.1    polar
//------------------------------------------------------------------------------
// 1.1.1
// polar
//   struct: 2D polar coordinate system (r, θ). An orthogonal curvilinear
// system where position is specified by radial distance r and angle θ from
// the positive x-axis.
template<typename ValueType = double>
struct polar
{
    // ---- type aliases -------------------------------------------------------

    typedef ValueType                          value_type;
    typedef linalg::vector<value_type, 2>      point_type;
    typedef linalg::vector<value_type, 2>      vector_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension      = 2;
    static D_CONSTEXPR_VAR bool        is_cartesian   = false;
    static D_CONSTEXPR_VAR bool        is_polar       = true;
    static D_CONSTEXPR_VAR bool        is_cylindrical = false;
    static D_CONSTEXPR_VAR bool        is_spherical   = false;
    static D_CONSTEXPR_VAR bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t R     = 0;
    static D_CONSTEXPR_VAR std::size_t THETA = 1;

    // ---- coordinate access --------------------------------------------------

    // r
    //   the radial coordinate.
    static D_CONSTEXPR value_type
    r(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }

    // theta
    //   the angular coordinate.
    static D_CONSTEXPR value_type
    theta(const point_type& _point) D_NOEXCEPT
    {
        return _point[THETA];
    }

    // ---- point construction -------------------------------------------------

    // origin
    //   the origin (r = 0, θ = 0).
    static D_CONSTEXPR point_type
    origin() D_NOEXCEPT
    {
        return point_type();
    }

    // make_point
    //   a polar point from (r, θ).
    static D_CONSTEXPR_CPP14 point_type
    make_point(
        value_type _r,
        value_type _theta
    ) D_NOEXCEPT
    {
        return point_type(_r, _theta);
    }

    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   the scale factors (h_r, h_θ) = (1, r) at a point.
    static D_CONSTEXPR_CPP14 vector_type
    scale_factors(const point_type& _point) D_NOEXCEPT
    {
        return vector_type(static_cast<value_type>(1), _point[R]);
    }

    // jacobian
    //   the Jacobian determinant |∂(x,y)/∂(r,θ)| = r.
    static D_CONSTEXPR value_type
    jacobian(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }

    // area_element
    //   the area element scale factor dA/(dr dθ) = r.
    static D_CONSTEXPR value_type
    area_element(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }

    // gradient_scale
    //   (1, 1/r), the factors of the gradient's components; (1, 0) at the
    // origin.
    static D_CONSTEXPR_CPP14 vector_type
    gradient_scale(const point_type& _point) D_NOEXCEPT
    {
        const value_type rv = _point[R];

        return (rv == static_cast<value_type>(0))
                   ? vector_type(static_cast<value_type>(1),
                                 static_cast<value_type>(0))
                   : vector_type(static_cast<value_type>(1),
                                 static_cast<value_type>(1) / rv);
    }

    // ---- coordinate conversion ----------------------------------------------

    // to_cartesian
    //   (r, θ) -> (x, y) = (r cos θ, r sin θ).
    static D_CONSTEXPR_CPP20 point_type
    to_cartesian(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_polar_to_cartesian_auto(result.begin(), _point.data());

        return result;
    }

    // from_cartesian
    //   (x, y) -> (r, θ) = (√(x² + y²), atan2(y, x)).
    static D_CONSTEXPR_CPP20 point_type
    from_cartesian(const point_type& _cart) D_NOEXCEPT
    {
        point_type result;

        d_coord_polar_from_cartesian_auto(result.begin(), _cart.data());

        return result;
    }

    // ---- metric -------------------------------------------------------------

    // distance
    //   the Euclidean distance between two polar points, by the law of
    // cosines: √(r₁² + r₂² - 2 r₁ r₂ cos(θ₁ - θ₂)).
    static D_CONSTEXPR_CPP20 value_type
    distance(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return d_coord_polar_distance_auto(_a.data(), _b.data());
    }

    // norm
    //   the distance from the origin: r.
    static D_CONSTEXPR value_type
    norm(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }

    // ---- canonical forms ----------------------------------------------------

    // normalize_angle
    //   _theta in [0, 2π): its remainder by 2π, a negative one moved up a
    // turn (exact).
    static D_CONSTEXPR_CPP14 value_type
    normalize_angle(value_type _theta) D_NOEXCEPT
    {
        return d_coord_normalize_angle(_theta);
    }

    // canonicalize
    //   the same point with r >= 0 (a negative r turned a half turn round)
    // and θ in [0, 2π).
    static D_CONSTEXPR_CPP14 point_type
    canonicalize(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_polar_canonicalize(result.begin(), _point.data());

        return result;
    }
};

// the static constants' definitions (a C++98 program that takes one's
// address needs one; from C++17 the in-class declarations are definitions)
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t polar<ValueType>::dimension;
template<typename ValueType>
D_CONSTEXPR_VAR bool polar<ValueType>::is_cartesian;
template<typename ValueType>
D_CONSTEXPR_VAR bool polar<ValueType>::is_polar;
template<typename ValueType>
D_CONSTEXPR_VAR bool polar<ValueType>::is_cylindrical;
template<typename ValueType>
D_CONSTEXPR_VAR bool polar<ValueType>::is_spherical;
template<typename ValueType>
D_CONSTEXPR_VAR bool polar<ValueType>::is_orthogonal;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t polar<ValueType>::R;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t polar<ValueType>::THETA;


//==============================================================================
// 2.  TYPE ALIASES
//==============================================================================


// polar_f, polar_d, polar_ld
//   type: the polar system in float, double and long double.
typedef polar<float>       polar_f;
typedef polar<double>      polar_d;
typedef polar<long double> polar_ld;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// polar_point
//   type: a point in polar coordinates (C++11: an alias template).
template<typename ValueType = double>
using polar_point = typename polar<ValueType>::point_type;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_POLAR_HPP
