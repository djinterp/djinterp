/******************************************************************************
* djinterp [math]                                            spherical.hpp
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
* STRUCTURAL INTERFACE (for coordinate traits):
*   - value_type, point_type
*   - static constexpr dimension = 3
*   - static constexpr bool is_cartesian   = false
*   - static constexpr bool is_polar       = false
*   - static constexpr bool is_cylindrical = false
*   - static constexpr bool is_spherical   = true
*   - static constexpr bool is_orthogonal  = true
*   - scale_factors(point), jacobian(point)
*   - to_cartesian(point), from_cartesian(point)
*
* 
* path:      /inc/djinterp/math/coordinate/spherical.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATH_COORDINATE_SPHERICAL_
#define DJINTERP_MATH_COORDINATE_SPHERICAL_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../math.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    SPHERICAL COORDINATE SYSTEM
// ============================================================================

// spherical
//   struct: 3D spherical coordinate system (r, θ, φ).
// An orthogonal curvilinear system where position is specified by
// radial distance r, polar angle θ from the z-axis, and azimuthal
// angle φ in the xy-plane. Uses the ISO/physics convention.
template<typename _ValueType = double>
struct spherical
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _ValueType;
    using point_type = std::array<value_type, 3>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      = 3;
    static constexpr bool        is_cartesian   = false;
    static constexpr bool        is_polar       = false;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical   = true;
    static constexpr bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static constexpr std::size_t R     = 0;
    static constexpr std::size_t THETA = 1;
    static constexpr std::size_t PHI   = 2;

    // ---- coordinate access --------------------------------------------------

    // r
    //   returns the radial distance from the origin.
    static constexpr value_type
    r
    (
        const point_type& _point
    ) noexcept
    {
        return _point[R];
    }

    // theta
    //   returns the polar angle from the positive z-axis.
    static constexpr value_type
    theta
    (
        const point_type& _point
    ) noexcept
    {
        return _point[THETA];
    }

    // phi
    //   returns the azimuthal angle in the xy-plane.
    static constexpr value_type
    phi
    (
        const point_type& _point
    ) noexcept
    {
        return _point[PHI];
    }


    // ---- point construction -------------------------------------------------

    // origin
    //   returns the origin (r = 0, θ = 0, φ = 0).
    static constexpr point_type
    origin
    () noexcept
    {
        return {{ static_cast<value_type>(0),
                  static_cast<value_type>(0),
                  static_cast<value_type>(0) }};
    }

    // make_point
    //   constructs a spherical point from (r, θ, φ).
    static constexpr point_type
    make_point
    (
        value_type _r,
        value_type _theta,
        value_type _phi
    ) noexcept
    {
        return {{ _r, _theta, _phi }};
    }


    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   returns the scale factors (h_r, h_θ, h_φ) at a given point.
    // h_r = 1, h_θ = r, h_φ = r sin θ.
    static std::array<value_type, 3>
    scale_factors
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];
        value_type st = std::sin(_point[THETA]);

        return {{ static_cast<value_type>(1),
                  rv,
                  rv * st }};
    }

    // jacobian
    //   returns the Jacobian determinant
    // |∂(x,y,z)/∂(r,θ,φ)| = r² sin θ.
    static value_type
    jacobian
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];

        return rv * rv * std::sin(_point[THETA]);
    }

    // volume_element
    //   returns the volume element scale factor
    // dV/(dr dθ dφ) = r² sin θ.
    static value_type
    volume_element
    (
        const point_type& _point
    ) noexcept
    {
        return jacobian(_point);
    }

    // solid_angle_element
    //   returns the solid angle element dΩ/(dθ dφ) = sin θ.
    // Useful for integrating over the unit sphere.
    static value_type
    solid_angle_element
    (
        const point_type& _point
    ) noexcept
    {
        return std::sin(_point[THETA]);
    }


    // ---- coordinate conversion (Cartesian) ----------------------------------

    // to_cartesian
    //   converts (r, θ, φ) -> (x, y, z).
    // x = r sin θ cos φ, y = r sin θ sin φ, z = r cos θ.
    static point_type
    to_cartesian
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];
        value_type st = std::sin(_point[THETA]);
        value_type ct = std::cos(_point[THETA]);

        return {{ rv * st * std::cos(_point[PHI]),
                  rv * st * std::sin(_point[PHI]),
                  rv * ct }};
    }

    // from_cartesian
    //   converts (x, y, z) -> (r, θ, φ).
    // r = √(x²+y²+z²), θ = acos(z/r), φ = atan2(y, x).
    static point_type
    from_cartesian
    (
        const point_type& _cart
    ) noexcept
    {
        value_type xv = _cart[0];
        value_type yv = _cart[1];
        value_type zv = _cart[2];

        value_type rv = std::sqrt(xv * xv + yv * yv + zv * zv);

        // handle origin: θ and φ are undefined, default to 0
        if (rv == static_cast<value_type>(0))
        {
            return origin();
        }

        return {{ rv,
                  std::acos(zv / rv),
                  std::atan2(yv, xv) }};
    }


    // ---- coordinate conversion (cylindrical) --------------------------------

    // to_cylindrical
    //   converts (r, θ, φ) -> (ρ, φ, z).
    // ρ = r sin θ, φ = φ, z = r cos θ.
    static point_type
    to_cylindrical
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];

        return {{ rv * std::sin(_point[THETA]),
                  _point[PHI],
                  rv * std::cos(_point[THETA]) }};
    }

    // from_cylindrical
    //   converts (ρ, φ, z) -> (r, θ, φ).
    // r = √(ρ² + z²), θ = atan2(ρ, z), φ = φ.
    static point_type
    from_cylindrical
    (
        const point_type& _cyl
    ) noexcept
    {
        value_type rv = _cyl[0];
        value_type zv = _cyl[2];

        return {{ std::sqrt(rv * rv + zv * zv),
                  std::atan2(rv, zv),
                  _cyl[1] }};
    }


    // ---- metric operations --------------------------------------------------

    // distance
    //   returns the Euclidean distance between two spherical points.
    // Uses the spherical law of cosines.
    static value_type
    distance
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        // convert via Cartesian for numerical stability
        point_type ca = to_cartesian(_a);
        point_type cb = to_cartesian(_b);

        value_type dx = ca[0] - cb[0];
        value_type dy = ca[1] - cb[1];
        value_type dz = ca[2] - cb[2];

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // norm
    //   returns the distance from the origin (simply r).
    static constexpr value_type
    norm
    (
        const point_type& _point
    ) noexcept
    {
        return _point[R];
    }

    // angular_distance
    //   returns the great-circle angular distance between two points
    // on the same sphere (ignoring radius differences).
    // Uses the Vincenty formula for numerical stability.
    static value_type
    angular_distance
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        value_type st1 = std::sin(_a[THETA]);
        value_type ct1 = std::cos(_a[THETA]);
        value_type st2 = std::sin(_b[THETA]);
        value_type ct2 = std::cos(_b[THETA]);
        value_type dp  = _a[PHI] - _b[PHI];
        value_type cdp = std::cos(dp);
        value_type sdp = std::sin(dp);

        // numerator: Vincenty formula
        value_type n1 = st2 * sdp;
        value_type n2 = st1 * ct2 - ct1 * st2 * cdp;
        value_type num = std::sqrt(n1 * n1 + n2 * n2);

        // denominator
        value_type den = ct1 * ct2 + st1 * st2 * cdp;

        return std::atan2(num, den);
    }

    // canonicalize
    //   returns point with r >= 0, θ in [0, π], φ in [0, 2π).
    static point_type
    canonicalize
    (
        const point_type& _point
    ) noexcept
    {
        constexpr value_type pi =
            static_cast<value_type>(3.14159265358979323846L);
        constexpr value_type two_pi =
            static_cast<value_type>(2) * pi;

        value_type rv = _point[R];
        value_type tv = _point[THETA];
        value_type pv = _point[PHI];

        // negative r: flip r and add π to θ
        if (rv < static_cast<value_type>(0))
        {
            rv = -rv;
            tv = pi - tv;
        }

        // normalize θ to [0, π]
        tv = std::fmod(tv, two_pi);

        if (tv < static_cast<value_type>(0))
        {
            tv += two_pi;
        }

        if (tv > pi)
        {
            tv = two_pi - tv;
            pv += pi;
        }

        // normalize φ to [0, 2π)
        pv = std::fmod(pv, two_pi);

        if (pv < static_cast<value_type>(0))
        {
            pv += two_pi;
        }

        return {{ rv, tv, pv }};
    }


    // ---- differential geometry helpers --------------------------------------

    // gradient_scale
    //   returns (1/h_r, 1/h_θ, 1/h_φ) for computing gradients.
    // ∇f = (∂f/∂r) r̂ + (1/r)(∂f/∂θ) θ̂ + (1/(r sinθ))(∂f/∂φ) φ̂
    static std::array<value_type, 3>
    gradient_scale
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];
        value_type st = std::sin(_point[THETA]);

        value_type inv_r = (rv == static_cast<value_type>(0))
            ? static_cast<value_type>(0)
            : static_cast<value_type>(1) / rv;

        value_type inv_r_st =
            ( (rv == static_cast<value_type>(0)) ||
              (st == static_cast<value_type>(0)) )
                ? static_cast<value_type>(0)
                : static_cast<value_type>(1) / (rv * st);

        return {{ static_cast<value_type>(1),
                  inv_r,
                  inv_r_st }};
    }

    // surface_area_element
    //   returns the surface area element on a sphere of radius r.
    // dA = r² sin θ dθ dφ.
    static value_type
    surface_area_element
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];

        return rv * rv * std::sin(_point[THETA]);
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// spherical_f
//   type: spherical coordinate system with float precision.
using spherical_f = spherical<float>;

// spherical_d
//   type: spherical coordinate system with double precision.
using spherical_d = spherical<double>;

// spherical_ld
//   type: spherical coordinate system with long double precision.
using spherical_ld = spherical<long double>;

// spherical_point
//   type: point in spherical coordinates.
template<typename _T = double>
using spherical_point = typename spherical<_T>::point_type;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_SPHERICAL_