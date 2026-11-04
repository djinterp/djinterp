/*******************************************************************************
* djinterp [cpp/math]                                              spherical.hpp
*
* Compile-time 3D spherical coordinate system (r, θ, φ).
*   Provides the spherical coordinate system type, point representation,
* scale factors, differential geometry infrastructure, and conversion
* to/from Cartesian and cylindrical coordinates.
*
* CONVENTION (ISO / physics):
*   r     - radial distance from origin          [0, ∞)
*   θ     - polar angle from positive z-axis     [0, π]
*   φ     - azimuthal angle from positive x-axis [0, 2π)
*
* SCALE FACTORS:
*   h_r = 1,  h_θ = r,  h_φ = r sin θ
*
* LINE ELEMENT:
*   ds² = dr² + r² dθ² + r² sin²θ dφ²
*
* VOLUME ELEMENT:
*   dV = r² sin θ dr dθ dφ
*
* CONVERSION (to Cartesian):
*   x = r sin θ cos φ,  y = r sin θ sin φ,  z = r cos θ
*
* CONVERSION (from Cartesian):
*   r = √(x² + y² + z²),  θ = acos(z/r),  φ = atan2(y, x)
*
* A FACE OVER THE C CORE:
*   The conversions, distances, measures and canonical forms are
* c/math/coord.h's, through its _auto names: the C library's functions at
* run time at every level, constant expressions from C++20, and through
* double for an element type other than float, double and long double. A
* point is a linalg::vector<value_type, 3>, written {{a, b, c}}. Everything
* compiles from C++98.
*
*
* path:      /inc/djinterp/cpp/modules/math/coordinate/spherical.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SPHERICAL COORDINATE SYSTEM
    -------------------------------
    1.  spherical
2.  TYPE ALIASES
    ------------
*/

#ifndef DJINTERP_MATH_COORDINATE_SPHERICAL_HPP
#define DJINTERP_MATH_COORDINATE_SPHERICAL_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../../c/modules/math/coord.h"     // the C core: d_coord_*
#include "../linear_algebra/vector.hpp"           // linalg::vector


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE SPHERICAL COORDINATE SYSTEM
//==============================================================================


// 1.1    spherical
//------------------------------------------------------------------------------
// 1.1.1
// spherical
//   struct: 3D spherical coordinate system (r, θ, φ), ISO / physics
// convention: θ the polar angle from +z, φ the azimuth from +x.
template<typename ValueType = double>
struct spherical
{
    // ---- type aliases -------------------------------------------------------

    typedef ValueType                          value_type;
    typedef linalg::vector<value_type, 3>      point_type;
    typedef linalg::vector<value_type, 3>      vector_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension      = 3;
    static D_CONSTEXPR_VAR bool        is_cartesian   = false;
    static D_CONSTEXPR_VAR bool        is_polar       = false;
    static D_CONSTEXPR_VAR bool        is_cylindrical = false;
    static D_CONSTEXPR_VAR bool        is_spherical   = true;
    static D_CONSTEXPR_VAR bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t R     = 0;
    static D_CONSTEXPR_VAR std::size_t THETA = 1;
    static D_CONSTEXPR_VAR std::size_t PHI   = 2;

    // ---- coordinate access --------------------------------------------------

    // r
    //   the radial distance from the origin.
    static D_CONSTEXPR value_type
    r(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }
    // theta
    //   the polar angle from +z.
    static D_CONSTEXPR value_type
    theta(const point_type& _point) D_NOEXCEPT
    {
        return _point[THETA];
    }
    // phi
    //   the azimuthal angle from +x.
    static D_CONSTEXPR value_type
    phi(const point_type& _point) D_NOEXCEPT
    {
        return _point[PHI];
    }

    // ---- point construction -------------------------------------------------

    // origin
    //   the origin.
    static D_CONSTEXPR point_type
    origin() D_NOEXCEPT
    {
        return point_type();
    }

    // make_point
    //   a spherical point from (r, θ, φ).
    static D_CONSTEXPR_CPP14 point_type
    make_point(
        value_type _r,
        value_type _theta,
        value_type _phi
    ) D_NOEXCEPT
    {
        return point_type(_r, _theta, _phi);
    }

    // ---- scale factors and elements -----------------------------------------

    // scale_factors
    //   the scale factors (h_r, h_θ, h_φ) = (1, r, r sin θ) at a point.
    static D_CONSTEXPR_CPP20 vector_type
    scale_factors(const point_type& _point) D_NOEXCEPT
    {
        vector_type result;

        d_coord_spherical_scale_factors_auto(result.begin(), _point.data());

        return result;
    }

    // jacobian
    //   the Jacobian determinant r² sin θ.
    static D_CONSTEXPR_CPP20 value_type
    jacobian(const point_type& _point) D_NOEXCEPT
    {
        return d_coord_spherical_jacobian_auto(_point.data());
    }

    // volume_element
    //   the volume element scale factor dV/(dr dθ dφ) = r² sin θ.
    static D_CONSTEXPR_CPP20 value_type
    volume_element(const point_type& _point) D_NOEXCEPT
    {
        return jacobian(_point);
    }

    // solid_angle_element
    //   the solid angle element dΩ/(dθ dφ) = sin θ: the Jacobian at r = 1,
    // exactly.
    static D_CONSTEXPR_CPP20 value_type
    solid_angle_element(const point_type& _point) D_NOEXCEPT
    {
        return jacobian(point_type(static_cast<value_type>(1),
                                   _point[THETA],
                                   static_cast<value_type>(0)));
    }

    // surface_area_element
    //   the area element on a sphere of radius r, dA/(dθ dφ) = r² sin θ.
    static D_CONSTEXPR_CPP20 value_type
    surface_area_element(const point_type& _point) D_NOEXCEPT
    {
        return jacobian(_point);
    }

    // gradient_scale
    //   (1, 1/r, 1/(r sin θ)), 0 where the denominator is.
    static D_CONSTEXPR_CPP20 vector_type
    gradient_scale(const point_type& _point) D_NOEXCEPT
    {
        vector_type result;

        d_coord_spherical_gradient_scale_auto(result.begin(), _point.data());

        return result;
    }

    // ---- coordinate conversion ----------------------------------------------

    // to_cartesian
    //   (r, θ, φ) -> (r sin θ cos φ, r sin θ sin φ, r cos θ).
    static D_CONSTEXPR_CPP20 point_type
    to_cartesian(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_spherical_to_cartesian_auto(result.begin(), _point.data());

        return result;
    }

    // from_cartesian
    //   (x, y, z) -> (r, atan2(√(x² + y²), z), atan2(y, x)); the origin for
    // the origin.
    static D_CONSTEXPR_CPP20 point_type
    from_cartesian(const point_type& _cart) D_NOEXCEPT
    {
        point_type result;

        d_coord_spherical_from_cartesian_auto(result.begin(), _cart.data());

        return result;
    }

    // to_cylindrical
    //   (r, θ, φ) -> (r sin θ, φ, r cos θ).
    static D_CONSTEXPR_CPP20 point_type
    to_cylindrical(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_spherical_to_cylindrical_auto(result.begin(), _point.data());

        return result;
    }

    // from_cylindrical
    //   (ρ, φ, z) -> (√(ρ² + z²), atan2(ρ, z), φ).
    static D_CONSTEXPR_CPP20 point_type
    from_cylindrical(const point_type& _cyl) D_NOEXCEPT
    {
        point_type result;

        d_coord_cylindrical_to_spherical_auto(result.begin(), _cyl.data());

        return result;
    }

    // ---- metric -------------------------------------------------------------

    // distance
    //   the Euclidean distance, through the cartesian forms.
    static D_CONSTEXPR_CPP20 value_type
    distance(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return d_coord_spherical_distance_auto(_a.data(), _b.data());
    }

    // norm
    //   the distance from the origin: r.
    static D_CONSTEXPR value_type
    norm(const point_type& _point) D_NOEXCEPT
    {
        return _point[R];
    }

    // angular_distance
    //   the angle between two directions, by Vincenty's arc-tangent form,
    // accurate at every angle.
    static D_CONSTEXPR_CPP20 value_type
    angular_distance(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return d_coord_spherical_angular_distance_auto(_a.data(), _b.data());
    }

    // ---- canonical forms ----------------------------------------------------

    // canonicalize
    //   the same point with r >= 0, θ in [0, π] and φ in [0, 2π) (exact).
    static D_CONSTEXPR_CPP14 point_type
    canonicalize(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_spherical_canonicalize(result.begin(), _point.data());

        return result;
    }
};

// the static constants' definitions (a C++98 program that takes one's
// address needs one; from C++17 the in-class declarations are definitions)
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t spherical<ValueType>::dimension;
template<typename ValueType>
D_CONSTEXPR_VAR bool spherical<ValueType>::is_cartesian;
template<typename ValueType>
D_CONSTEXPR_VAR bool spherical<ValueType>::is_polar;
template<typename ValueType>
D_CONSTEXPR_VAR bool spherical<ValueType>::is_cylindrical;
template<typename ValueType>
D_CONSTEXPR_VAR bool spherical<ValueType>::is_spherical;
template<typename ValueType>
D_CONSTEXPR_VAR bool spherical<ValueType>::is_orthogonal;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t spherical<ValueType>::R;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t spherical<ValueType>::THETA;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t spherical<ValueType>::PHI;


//==============================================================================
// 2.  TYPE ALIASES
//==============================================================================


// spherical_f, spherical_d, spherical_ld
//   type: the spherical system in float, double and long double.
typedef spherical<float>       spherical_f;
typedef spherical<double>      spherical_d;
typedef spherical<long double> spherical_ld;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// spherical_point
//   type: a point in spherical coordinates (C++11: an alias template).
template<typename ValueType = double>
using spherical_point = typename spherical<ValueType>::point_type;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_SPHERICAL_HPP
