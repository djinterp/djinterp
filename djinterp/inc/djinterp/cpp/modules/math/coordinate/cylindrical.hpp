/*******************************************************************************
* djinterp [cpp/math]                                            cylindrical.hpp
*
* Compile-time 3D cylindrical coordinate system (ρ, φ, z).
*   Provides the cylindrical coordinate system type, point representation,
* scale factors, differential geometry infrastructure, and conversion
* to/from Cartesian and spherical coordinates.
*
* COORDINATES:
*   ρ (rho)  - radial distance from z-axis       [0, ∞)
*   φ (phi)  - azimuthal angle from x-axis        [0, 2π)
*   z        - height along z-axis                 (-∞, ∞)
*
* SCALE FACTORS:
*   h_ρ = 1,  h_φ = ρ,  h_z = 1
*
* LINE ELEMENT:
*   ds² = dρ² + ρ² dφ² + dz²
*
* VOLUME ELEMENT:
*   dV = ρ dρ dφ dz
*
* CONVERSION (to Cartesian):
*   x = ρ cos φ,  y = ρ sin φ,  z = z
*
* CONVERSION (from Cartesian):
*   ρ = √(x² + y²),  φ = atan2(y, x),  z = z
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
* path:      /inc/djinterp/cpp/modules/math/coordinate/cylindrical.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE CYLINDRICAL COORDINATE SYSTEM
    ---------------------------------
    1.  cylindrical
2.  TYPE ALIASES
    ------------
*/

#ifndef DJINTERP_MATH_COORDINATE_CYLINDRICAL_HPP
#define DJINTERP_MATH_COORDINATE_CYLINDRICAL_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../../../../c/modules/math/coord.h"                   // the C core: d_coord_*
#if (defined(__cplusplus))
    #include "../../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
#include "../linear_algebra/vector.hpp"           // linalg::vector


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE CYLINDRICAL COORDINATE SYSTEM
//==============================================================================


// 1.1    cylindrical
//------------------------------------------------------------------------------
// 1.1.1
// cylindrical
//   struct: 3D cylindrical coordinate system (ρ, φ, z): polar
// coordinates in the xy-plane with a height z.
template<typename ValueType = double>
struct cylindrical
{
    // ---- type aliases -------------------------------------------------------

    typedef ValueType                          value_type;
    typedef linalg::vector<value_type, 3>      point_type;
    typedef linalg::vector<value_type, 3>      vector_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension      = 3;
    static D_CONSTEXPR_VAR bool        is_cartesian   = false;
    static D_CONSTEXPR_VAR bool        is_polar       = false;
    static D_CONSTEXPR_VAR bool        is_cylindrical = true;
    static D_CONSTEXPR_VAR bool        is_spherical   = false;
    static D_CONSTEXPR_VAR bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t RHO   = 0;
    static D_CONSTEXPR_VAR std::size_t PHI   = 1;
    static D_CONSTEXPR_VAR std::size_t Z     = 2;

    // ---- coordinate access --------------------------------------------------

    // rho
    //   the radial distance from the z axis.
    static D_CONSTEXPR value_type
    rho(const point_type& _point) D_NOEXCEPT
    {
        return _point[RHO];
    }
    // phi
    //   the azimuthal angle.
    static D_CONSTEXPR value_type
    phi(const point_type& _point) D_NOEXCEPT
    {
        return _point[PHI];
    }
    // z
    //   the height.
    static D_CONSTEXPR value_type
    z(const point_type& _point) D_NOEXCEPT
    {
        return _point[Z];
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
    //   a cylindrical point from (ρ, φ, z).
    static D_CONSTEXPR_CPP14 point_type
    make_point(
        value_type _rho,
        value_type _phi,
        value_type _z
    ) D_NOEXCEPT
    {
        return point_type(_rho, _phi, _z);
    }

    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   the scale factors (h_ρ, h_φ, h_z) = (1, ρ, 1) at a point.
    static D_CONSTEXPR_CPP14 vector_type
    scale_factors(const point_type& _point) D_NOEXCEPT
    {
        return vector_type(static_cast<value_type>(1),
                           _point[RHO],
                           static_cast<value_type>(1));
    }

    // jacobian
    //   the Jacobian determinant |∂(x,y,z)/∂(ρ,φ,z)| = ρ.
    static D_CONSTEXPR value_type
    jacobian(const point_type& _point) D_NOEXCEPT
    {
        return _point[RHO];
    }

    // volume_element
    //   the volume element scale factor dV/(dρ dφ dz) = ρ.
    static D_CONSTEXPR value_type
    volume_element(const point_type& _point) D_NOEXCEPT
    {
        return _point[RHO];
    }

    // gradient_scale
    //   (1, 1/ρ, 1), the factors of the gradient's components; 0 for 1/ρ on
    // the axis.
    static D_CONSTEXPR_CPP14 vector_type
    gradient_scale(const point_type& _point) D_NOEXCEPT
    {
        const value_type rv = _point[RHO];

        return vector_type(static_cast<value_type>(1),
                           (rv == static_cast<value_type>(0))
                               ? static_cast<value_type>(0)
                               : (static_cast<value_type>(1) / rv),
                           static_cast<value_type>(1));
    }

    // ---- coordinate conversion ----------------------------------------------

    // to_cartesian
    //   (ρ, φ, z) -> (ρ cos φ, ρ sin φ, z).
    static D_CONSTEXPR_CPP20 point_type
    to_cartesian(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_cylindrical_to_cartesian_auto(result.begin(), _point.data());

        return result;
    }

    // from_cartesian
    //   (x, y, z) -> (√(x² + y²), atan2(y, x), z).
    static D_CONSTEXPR_CPP20 point_type
    from_cartesian(const point_type& _cart) D_NOEXCEPT
    {
        point_type result;

        d_coord_cylindrical_from_cartesian_auto(result.begin(), _cart.data());

        return result;
    }

    // to_spherical
    //   (ρ, φ, z) -> (√(ρ² + z²), atan2(ρ, z), φ).
    static D_CONSTEXPR_CPP20 point_type
    to_spherical(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_cylindrical_to_spherical_auto(result.begin(), _point.data());

        return result;
    }

    // from_spherical
    //   (r, θ, φ) -> (r sin θ, φ, r cos θ).
    static D_CONSTEXPR_CPP20 point_type
    from_spherical(const point_type& _sph) D_NOEXCEPT
    {
        point_type result;

        d_coord_spherical_to_cylindrical_auto(result.begin(), _sph.data());

        return result;
    }

    // ---- metric -------------------------------------------------------------

    // distance
    //   the Euclidean distance, √(ρ₁² + ρ₂² - 2 ρ₁ ρ₂ cos(φ₁ - φ₂) + Δz²).
    static D_CONSTEXPR_CPP20 value_type
    distance(
        const point_type& _a,
        const point_type& _b
    ) D_NOEXCEPT
    {
        return d_coord_cylindrical_distance_auto(_a.data(), _b.data());
    }

    // norm
    //   the distance from the origin, √(ρ² + z²).
    static D_CONSTEXPR_CPP20 value_type
    norm(const point_type& _point) D_NOEXCEPT
    {
        return d_coord_cylindrical_norm_auto(_point.data());
    }

    // ---- canonical forms ----------------------------------------------------

    // normalize_angle
    //   _phi in [0, 2π) (exact).
    static D_CONSTEXPR_CPP14 value_type
    normalize_angle(value_type _phi) D_NOEXCEPT
    {
        return d_coord_normalize_angle(_phi);
    }

    // canonicalize
    //   the same point with ρ >= 0 and φ in [0, 2π).
    static D_CONSTEXPR_CPP14 point_type
    canonicalize(const point_type& _point) D_NOEXCEPT
    {
        point_type result;

        d_coord_cylindrical_canonicalize(result.begin(), _point.data());

        return result;
    }
};

// the static constants' definitions (a C++98 program that takes one's
// address needs one; from C++17 the in-class declarations are definitions)
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t cylindrical<ValueType>::dimension;
template<typename ValueType>
D_CONSTEXPR_VAR bool cylindrical<ValueType>::is_cartesian;
template<typename ValueType>
D_CONSTEXPR_VAR bool cylindrical<ValueType>::is_polar;
template<typename ValueType>
D_CONSTEXPR_VAR bool cylindrical<ValueType>::is_cylindrical;
template<typename ValueType>
D_CONSTEXPR_VAR bool cylindrical<ValueType>::is_spherical;
template<typename ValueType>
D_CONSTEXPR_VAR bool cylindrical<ValueType>::is_orthogonal;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t cylindrical<ValueType>::RHO;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t cylindrical<ValueType>::PHI;
template<typename ValueType>
D_CONSTEXPR_VAR std::size_t cylindrical<ValueType>::Z;


//==============================================================================
// 2.  TYPE ALIASES
//==============================================================================


// cylindrical_f, cylindrical_d, cylindrical_ld
//   type: the cylindrical system in float, double and long double.
typedef cylindrical<float>       cylindrical_f;
typedef cylindrical<double>      cylindrical_d;
typedef cylindrical<long double> cylindrical_ld;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// cylindrical_point
//   type: a point in cylindrical coordinates (C++11: an alias template).
template<typename ValueType = double>
using cylindrical_point = typename cylindrical<ValueType>::point_type;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_CYLINDRICAL_HPP
