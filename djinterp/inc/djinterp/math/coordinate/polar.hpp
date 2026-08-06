/******************************************************************************
* djinterp [math]                                                polar.hpp
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
* STRUCTURAL INTERFACE (for coordinate traits):
*   - value_type, point_type
*   - static constexpr dimension = 2
*   - static constexpr bool is_cartesian   = false
*   - static constexpr bool is_polar       = true
*   - static constexpr bool is_cylindrical = false
*   - static constexpr bool is_spherical   = false
*   - static constexpr bool is_orthogonal  = true
*   - scale_factors(point), jacobian(point)
*   - to_cartesian(point), from_cartesian(point)
*
* path:      /inc/djinterp/math/coordinate/polar.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATH_POLAR_
#define DJINTERP_MATH_POLAR_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
#include "../djinterp.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    POLAR COORDINATE SYSTEM
// ============================================================================

// polar
//   struct: 2D polar coordinate system (r, θ).
// An orthogonal curvilinear system where position is specified by
// radial distance r and angle θ from the positive x-axis.
template<typename _ValueType = double>
struct polar
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _ValueType;
    using point_type = std::array<value_type, 2>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      = 2;
    static constexpr bool        is_cartesian   = false;
    static constexpr bool        is_polar       = true;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical   = false;
    static constexpr bool        is_orthogonal  = true;

    // ---- axis indices -------------------------------------------------------

    static constexpr std::size_t R     = 0;
    static constexpr std::size_t THETA = 1;

    // ---- coordinate access --------------------------------------------------

    // r
    //   returns the radial coordinate.
    static constexpr value_type
    r
    (
        const point_type& _point
    ) noexcept
    {
        return _point[R];
    }

    // theta
    //   returns the angular coordinate.
    static constexpr value_type
    theta
    (
        const point_type& _point
    ) noexcept
    {
        return _point[THETA];
    }


    // ---- point construction -------------------------------------------------

    // origin
    //   returns the origin (r = 0, θ = 0).
    static constexpr point_type
    origin
    () noexcept
    {
        return {{ static_cast<value_type>(0),
                  static_cast<value_type>(0) }};
    }

    // make_point
    //   constructs a polar point from (r, θ).
    static constexpr point_type
    make_point
    (
        value_type _r,
        value_type _theta
    ) noexcept
    {
        return {{ _r, _theta }};
    }


    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   returns the scale factors (h_r, h_θ) at a given point.
    // h_r = 1, h_θ = r.
    static constexpr std::array<value_type, 2>
    scale_factors
    (
        const point_type& _point
    ) noexcept
    {
        return {{ static_cast<value_type>(1), _point[R] }};
    }

    // jacobian
    //   returns the Jacobian determinant |∂(x,y)/∂(r,θ)| = r.
    static constexpr value_type
    jacobian
    (
        const point_type& _point
    ) noexcept
    {
        return _point[R];
    }

    // area_element
    //   returns the area element scale factor dA/(dr dθ) = r.
    static constexpr value_type
    area_element
    (
        const point_type& _point
    ) noexcept
    {
        return _point[R];
    }


    // ---- coordinate conversion ----------------------------------------------

    // to_cartesian
    //   converts (r, θ) -> (x, y).
    // x = r cos θ, y = r sin θ.
    static point_type
    to_cartesian
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];
        value_type tv = _point[THETA];

        return {{ rv * std::cos(tv),
                  rv * std::sin(tv) }};
    }

    // from_cartesian
    //   converts (x, y) -> (r, θ).
    // r = √(x² + y²), θ = atan2(y, x).
    static point_type
    from_cartesian
    (
        const point_type& _cart
    ) noexcept
    {
        return {{ std::sqrt(_cart[0] * _cart[0] +
                            _cart[1] * _cart[1]),
                  std::atan2(_cart[1], _cart[0]) }};
    }


    // ---- metric operations --------------------------------------------------

    // distance
    //   returns the Euclidean distance between two polar points.
    // Uses the law of cosines: d² = r₁² + r₂² - 2r₁r₂cos(θ₁-θ₂).
    static value_type
    distance
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        value_type r1 = _a[R];
        value_type r2 = _b[R];
        value_type dt = _a[THETA] - _b[THETA];

        return std::sqrt(
            r1 * r1 + r2 * r2 -
            static_cast<value_type>(2) * r1 * r2 * std::cos(dt)
        );
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

    // normalize_angle
    //   wraps θ into [0, 2π).
    static value_type
    normalize_angle
    (
        value_type _theta
    ) noexcept
    {
        constexpr value_type two_pi =
            static_cast<value_type>(2) *
            static_cast<value_type>(3.14159265358979323846L);

        value_type result = std::fmod(_theta, two_pi);

        if (result < static_cast<value_type>(0))
        {
            result += two_pi;
        }

        return result;
    }

    // canonicalize
    //   returns point with r >= 0 and θ in [0, 2π).
    // If r < 0, negates r and adds π to θ.
    static point_type
    canonicalize
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];
        value_type tv = _point[THETA];

        if (rv < static_cast<value_type>(0))
        {
            rv = -rv;
            tv += static_cast<value_type>(3.14159265358979323846L);
        }

        return {{ rv, normalize_angle(tv) }};
    }


    // ---- differential geometry helpers --------------------------------------

    // gradient_scale
    //   returns (1/h_r, 1/h_θ) for computing gradients.
    // ∇f = (∂f/∂r) r̂ + (1/r)(∂f/∂θ) θ̂
    static std::array<value_type, 2>
    gradient_scale
    (
        const point_type& _point
    ) noexcept
    {
        value_type rv = _point[R];

        if (rv == static_cast<value_type>(0))
        {
            return {{ static_cast<value_type>(1),
                      static_cast<value_type>(0) }};
        }

        return {{ static_cast<value_type>(1),
                  static_cast<value_type>(1) / rv }};
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// polar_f
//   type: polar coordinate system with float precision.
using polar_f = polar<float>;

// polar_d
//   type: polar coordinate system with double precision.
using polar_d = polar<double>;

// polar_ld
//   type: polar coordinate system with long double precision.
using polar_ld = polar<long double>;

// polar_point
//   type: point in polar coordinates.
template<typename _T = double>
using polar_point = typename polar<_T>::point_type;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_POLAR_
