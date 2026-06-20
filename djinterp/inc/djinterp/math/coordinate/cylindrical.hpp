/******************************************************************************
* djinterp [math]                                          cylindrical.hpp
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
* STRUCTURAL INTERFACE (for coordinate traits):
*   - value_type, point_type
*   - static constexpr dimension = 3
*   - static constexpr bool is_cartesian   = false
*   - static constexpr bool is_polar       = false
*   - static constexpr bool is_cylindrical = true
*   - static constexpr bool is_spherical   = false
*   - static constexpr bool is_orthogonal  = true
*   - scale_factors(point), jacobian(point)
*   - to_cartesian(point), from_cartesian(point)
*
* 
* path:      /inc/djinterp/math/coordinate/cylindrical.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATH_COORDINATE_CYLINDRICAL_
#define DJINTERP_MATH_COORDINATE_CYLINDRICAL_ 1

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
// I.    CYLINDRICAL COORDINATE SYSTEM
// ============================================================================

// cylindrical
//   struct: 3D cylindrical coordinate system (ρ, φ, z).
// An orthogonal curvilinear system extending polar coordinates into
// three dimensions by adding a height coordinate z along the
// cylindrical axis.
template<typename _ValueType = double>
struct cylindrical
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _ValueType;
    using point_type = std::array<value_type, 3>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      = 3;
    static constexpr bool        is_cartesian   = false;
    static constexpr bool        is_polar       = false;
    static constexpr bool        is_cylindrical = true;
    static constexpr bool        is_spherical   = false;
    static constexpr bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static constexpr std::size_t RHO = 0;
    static constexpr std::size_t PHI = 1;
    static constexpr std::size_t Z   = 2;

    // ---- coordinate access --------------------------------------------------

    // rho
    //   returns the radial distance from the z-axis.
    static constexpr value_type
    rho
    (
        const point_type& _point
    ) noexcept
    {
        return _point[RHO];
    }

    // phi
    //   returns the azimuthal angle.
    static constexpr value_type
    phi
    (
        const point_type& _point
    ) noexcept
    {
        return _point[PHI];
    }

    // z
    //   returns the height coordinate.
    static constexpr value_type
    z
    (
        const point_type& _point
    ) noexcept
    {
        return _point[Z];
    }


    // ---- point construction -------------------------------------------------

    // origin
    //   returns the origin (ρ = 0, φ = 0, z = 0).
    static constexpr point_type
    origin
    () noexcept
    {
        return {{ static_cast<value_type>(0),
                  static_cast<value_type>(0),
                  static_cast<value_type>(0) }};
    }

    // make_point
    //   constructs a cylindrical point from (ρ, φ, z).
    static constexpr point_type
    make_point
    (
        value_type _rho,
        value_type _phi,
        value_type _z
    ) noexcept
    {
        return {{ _rho, _phi, _z }};
    }


    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   returns the scale factors (h_ρ, h_φ, h_z) at a given point.
    // h_ρ = 1, h_φ = ρ, h_z = 1.
    static constexpr std::array<value_type, 3>
    scale_factors
    (
        const point_type& _point
    ) noexcept
    {
        return {{ static_cast<value_type>(1),
                  _point[RHO],
                  static_cast<value_type>(1) }};
    }

    // jacobian
    //   returns the Jacobian determinant |∂(x,y,z)/∂(ρ,φ,z)| = ρ.
    static constexpr value_type
    jacobian
    (
        const point_type& _point
    ) noexcept
    {
        return _point[RHO];
    }

    // volume_element
    //   returns the volume element scale factor dV/(dρ dφ dz) = ρ.
    static constexpr value_type
    volume_element
    (
        const point_type& _point
    ) noexcept
    {
        return _point[RHO];
    }


    // ---- coordinate conversion (Cartesian) ----------------------------------

    // to_cartesian
    //   converts (ρ, φ, z) -> (x, y, z).
    // x = ρ cos φ, y = ρ sin φ, z = z.
    static point_type
    to_cartesian
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[RHO];
        value_type pv = _point[PHI];

        return {{ rv * std::cos(pv),
                  rv * std::sin(pv),
                  _point[Z] }};
    }

    // from_cartesian
    //   converts (x, y, z) -> (ρ, φ, z).
    // ρ = √(x² + y²), φ = atan2(y, x), z = z.
    static point_type
    from_cartesian
    (
        const point_type& _cart
    ) noexcept
    {
        return {{ std::sqrt(_cart[0] * _cart[0] +
                            _cart[1] * _cart[1]),
                  std::atan2(_cart[1], _cart[0]),
                  _cart[2] }};
    }


    // ---- coordinate conversion (spherical) ----------------------------------

    // to_spherical
    //   converts (ρ, φ, z) -> (r, θ, φ).
    // r = √(ρ² + z²), θ = atan2(ρ, z), φ = φ.
    static point_type
    to_spherical
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[RHO];
        value_type zv = _point[Z];

        return {{ std::sqrt(rv * rv + zv * zv),
                  std::atan2(rv, zv),
                  _point[PHI] }};
    }

    // from_spherical
    //   converts (r, θ, φ) -> (ρ, φ, z).
    // ρ = r sin θ, φ = φ, z = r cos θ.
    static point_type
    from_spherical
    (
        const point_type& _sph
    ) noexcept
    {
        value_type rv  = _sph[0];
        value_type tv  = _sph[1];

        return {{ rv * std::sin(tv),
                  _sph[2],
                  rv * std::cos(tv) }};
    }


    // ---- metric operations --------------------------------------------------

    // distance
    //   returns the Euclidean distance between two cylindrical points.
    // d² = ρ₁² + ρ₂² - 2ρ₁ρ₂cos(φ₁-φ₂) + (z₁-z₂)².
    static value_type
    distance
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        value_type r1  = _a[RHO];
        value_type r2  = _b[RHO];
        value_type dp  = _a[PHI] - _b[PHI];
        value_type dz  = _a[Z] - _b[Z];

        return std::sqrt(
            r1 * r1 + r2 * r2 -
            static_cast<value_type>(2) * r1 * r2 * std::cos(dp) +
            dz * dz
        );
    }

    // norm
    //   returns the distance from the origin: √(ρ² + z²).
    static value_type
    norm
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[RHO];
        value_type zv = _point[Z];

        return std::sqrt(rv * rv + zv * zv);
    }

    // normalize_angle
    //   wraps φ into [0, 2π).
    static value_type
    normalize_angle
    (
        value_type _phi
    ) noexcept
    {
        constexpr value_type two_pi =
            static_cast<value_type>(2) *
            static_cast<value_type>(3.14159265358979323846L);

        value_type result = std::fmod(_phi, two_pi);

        if (result < static_cast<value_type>(0))
        {
            result += two_pi;
        }

        return result;
    }

    // canonicalize
    //   returns point with ρ >= 0 and φ in [0, 2π).
    static point_type
    canonicalize
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[RHO];
        value_type pv = _point[PHI];

        if (rv < static_cast<value_type>(0))
        {
            rv = -rv;
            pv += static_cast<value_type>(3.14159265358979323846L);
        }

        return {{ rv, normalize_angle(pv), _point[Z] }};
    }


    // ---- differential geometry helpers --------------------------------------

    // gradient_scale
    //   returns (1/h_ρ, 1/h_φ, 1/h_z) for computing gradients.
    // ∇f = (∂f/∂ρ) ρ̂ + (1/ρ)(∂f/∂φ) φ̂ + (∂f/∂z) ẑ
    static std::array<value_type, 3>
    gradient_scale
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[RHO];

        value_type inv_rho = (rv == static_cast<value_type>(0))
            ? static_cast<value_type>(0)
            : static_cast<value_type>(1) / rv;

        return {{ static_cast<value_type>(1),
                  inv_rho,
                  static_cast<value_type>(1) }};
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// cylindrical_f
//   type: cylindrical coordinate system with float precision.
using cylindrical_f = cylindrical<float>;

// cylindrical_d
//   type: cylindrical coordinate system with double precision.
using cylindrical_d = cylindrical<double>;

// cylindrical_ld
//   type: cylindrical coordinate system with long double precision.
using cylindrical_ld = cylindrical<long double>;

// cylindrical_point
//   type: point in cylindrical coordinates.
template<typename _T = double>
using cylindrical_point = typename cylindrical<_T>::point_type;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_CYLINDRICAL_