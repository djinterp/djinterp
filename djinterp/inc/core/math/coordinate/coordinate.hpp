/******************************************************************************
* djinterp [maths]                                           coordinate.hpp
*
* Unified coordinate system header.
*   This header includes all coordinate sub-modules (Cartesian, polar,
* cylindrical, spherical) and provides cross-system conversion dispatch,
* the system-agnostic point wrapper, and differential geometry utilities
* that work with any orthogonal coordinate system.
*
* SUB-MODULES:
*   cartesian.hpp   - N-dimensional Cartesian (x, y, z, ...)
*   polar.hpp       - 2D polar (r, θ)
*   cylindrical.hpp - 3D cylindrical (ρ, φ, z)
*   spherical.hpp   - 3D spherical (r, θ, φ)
*
* PROVIDED BY THIS HEADER:
*   coord_point<System>        - typed point in a coordinate system
*   coord_convert<From, To>    - generic cross-system conversion
*   coord_cast<To>(from_point) - free function conversion
*   differential<System>       - differential operator helpers
*
* STRUCTURAL REQUIREMENTS FOR COORDINATE SYSTEMS:
*   A coordinate system type must have:
*     - value_type, point_type
*     - static constexpr dimension
*     - static constexpr bool is_cartesian
*     - static constexpr bool is_polar
*     - static constexpr bool is_cylindrical
*     - static constexpr bool is_spherical
*     - static constexpr bool is_orthogonal
*     - static to_cartesian(point_type) -> point_type
*     - static from_cartesian(point_type) -> point_type
*     - static scale_factors(point_type) -> array<value_type, dimension>
*     - static jacobian(point_type) -> value_type
*
* path:      /inc/maths/coordinate.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATHS_COORDINATE_
#define DJINTERP_MATHS_COORDINATE_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
#include "../djinterp.h"

// sub-module headers (usable independently)
#include "cartesian.hpp"
#include "polar.hpp"
#include "cylindrical.hpp"
#include "spherical.hpp"


NS_DJINTERP
NS_MATHS


// ============================================================================
// I.    TYPED POINT WRAPPER
// ============================================================================

// coord_point
//   struct: a point tagged with its coordinate system.
// Wraps a raw std::array with type information about which coordinate
// system the values represent. Prevents accidental mixing of coordinate
// values from different systems.
template<typename _System>
struct coord_point
{
    // ---- type aliases -------------------------------------------------------

    using system_type = _System;
    using value_type  = typename _System::value_type;
    using point_type  = typename _System::point_type;

    static constexpr std::size_t dimension = _System::dimension;

    // ---- data ---------------------------------------------------------------

    point_type data;

    // ---- construction -------------------------------------------------------

    constexpr coord_point() noexcept
        : data{}
    {
    }

    constexpr explicit coord_point(const point_type& _p) noexcept
        : data(_p)
    {
    }

    // ---- access -------------------------------------------------------------

    constexpr value_type  operator[](std::size_t _i) const noexcept
    {
        return data[_i];
    }

    constexpr value_type& operator[](std::size_t _i) noexcept
    {
        return data[_i];
    }

    constexpr const point_type& raw() const noexcept
    {
        return data;
    }

    constexpr point_type& raw() noexcept
    {
        return data;
    }

    // ---- coordinate system queries ------------------------------------------

    constexpr std::array<value_type, dimension>
    scale_factors
    () const noexcept
    {
        return _System::scale_factors(data);
    }

    constexpr value_type
    jacobian
    () const noexcept
    {
        return _System::jacobian(data);
    }

    // ---- conversion ---------------------------------------------------------

    // to_cartesian
    //   converts this point to Cartesian coordinates.
    auto
    to_cartesian
    () const noexcept
    {
        return coord_point<cartesian<dimension, value_type>>(
            _System::to_cartesian(data)
        );
    }
};


// ============================================================================
// II.   CROSS-SYSTEM CONVERSION
// ============================================================================

NS_INTERNAL

    // ---- same-system (identity) ---------------------------------------------

    // convert_impl
    //   helper: default implementation routes through Cartesian.
    // From -> Cartesian -> To.
    template<typename _From,
             typename _To,
             bool     _SameSystem = std::is_same<_From, _To>::value>
    struct convert_impl
    {
        using from_point = typename _From::point_type;
        using to_point   = typename _To::point_type;

        static to_point
        convert
        (
            const from_point& _point
        ) noexcept
        {
            // route through Cartesian as the universal pivot
            auto cart = _From::to_cartesian(_point);

            return _To::from_cartesian(cart);
        }
    };

    // identity specialization
    template<typename _System>
    struct convert_impl<_System, _System, true>
    {
        using point_type = typename _System::point_type;

        static constexpr point_type
        convert
        (
            const point_type& _point
        ) noexcept
        {
            return _point;
        }
    };

    // ---- direct conversion specializations ----------------------------------
    // These bypass the Cartesian pivot when direct formulas exist.

    // cylindrical -> spherical (direct)
    template<typename _T>
    struct convert_impl<cylindrical<_T>, spherical<_T>, false>
    {
        using from_point = typename cylindrical<_T>::point_type;
        using to_point   = typename spherical<_T>::point_type;

        static to_point
        convert
        (
            const from_point& _point
        ) noexcept
        {
            return cylindrical<_T>::to_spherical(_point);
        }
    };

    // spherical -> cylindrical (direct)
    template<typename _T>
    struct convert_impl<spherical<_T>, cylindrical<_T>, false>
    {
        using from_point = typename spherical<_T>::point_type;
        using to_point   = typename cylindrical<_T>::point_type;

        static to_point
        convert
        (
            const from_point& _point
        ) noexcept
        {
            return spherical<_T>::to_cylindrical(_point);
        }
    };

NS_END  // internal

// coord_convert
//   struct: converts a point between coordinate systems.
// Uses direct formulas when available (e.g. cylindrical <-> spherical),
// otherwise routes through Cartesian as a universal pivot.
//
// Usage:
//   auto cart_pt = coord_convert<polar<>, cartesian_2d<>>::convert(polar_pt);
template<typename _From,
         typename _To>
struct coord_convert
{
    using from_type  = _From;
    using to_type    = _To;
    using from_point = typename _From::point_type;
    using to_point   = typename _To::point_type;

    static_assert(_From::dimension == _To::dimension ||
                  _From::is_cartesian || _To::is_cartesian,
                  "coord_convert: dimension mismatch between "
                  "coordinate systems.");

    // convert
    //   converts a raw point from _From coordinates to _To coordinates.
    static to_point
    convert
    (
        const from_point& _point
    ) noexcept
    {
        return internal::convert_impl<_From, _To>::convert(_point);
    }

    // convert (typed point)
    //   converts a coord_point from _From to _To.
    static coord_point<_To>
    convert
    (
        const coord_point<_From>& _point
    ) noexcept
    {
        return coord_point<_To>(
            internal::convert_impl<_From, _To>::convert(_point.raw())
        );
    }
};

// coord_cast
//   function: free function for converting coord_point between systems.
// Usage:
//   auto cart = coord_cast<cartesian_2d<>>(polar_pt);
template<typename _To, typename _From>
coord_point<_To>
coord_cast
(
    const coord_point<_From>& _point
) noexcept
{
    return coord_convert<_From, _To>::convert(_point);
}

// coord_cast (raw array)
//   function: converts a raw point between coordinate systems.
template<typename _To, typename _From>
typename _To::point_type
coord_cast
(
    const typename _From::point_type& _point
) noexcept
{
    return coord_convert<_From, _To>::convert(_point);
}


// ============================================================================
// III.  DIFFERENTIAL GEOMETRY UTILITIES
// ============================================================================

// differential
//   struct: provides differential geometry operations for any
// orthogonal coordinate system. These are computed from the scale
// factors alone, making them coordinate-system-agnostic.
template<typename _System>
struct differential
{
    using value_type = typename _System::value_type;
    using point_type = typename _System::point_type;

    static constexpr std::size_t dim = _System::dimension;

    static_assert(_System::is_orthogonal,
                  "differential: requires an orthogonal coordinate "
                  "system.");

    // line_element_squared
    //   returns ds² = Σ (h_i dq_i)² for displacements dq.
    // The displacement array contains coordinate differentials.
    static value_type
    line_element_squared
    (
        const point_type& _point,
        const std::array<value_type, dim>& _dq
    ) noexcept
    {
        auto h = _System::scale_factors(_point);

        value_type ds2 = static_cast<value_type>(0);

        for (std::size_t i = 0; i < dim; ++i)
        {
            value_type term = h[i] * _dq[i];
            ds2 += term * term;
        }

        return ds2;
    }

    // line_element
    //   returns ds = √(ds²) for coordinate differentials dq.
    static value_type
    line_element
    (
        const point_type& _point,
        const std::array<value_type, dim>& _dq
    ) noexcept
    {
        return std::sqrt(line_element_squared(_point, _dq));
    }

    // volume_element
    //   returns the volume element dV = (∏ h_i) dq₁ dq₂ ... dqₙ.
    // This is the product of all scale factors (the Jacobian).
    static value_type
    volume_element
    (
        const point_type& _point
    ) noexcept
    {
        return _System::jacobian(_point);
    }

    // surface_element
    //   returns the surface element for a surface where coordinate
    // _omit_axis is held constant.
    // dA = (∏ h_i, i ≠ _omit_axis) dq₁ ... d̂q_k ... dqₙ.
    static value_type
    surface_element
    (
        const point_type& _point,
        std::size_t       _omit_axis
    ) noexcept
    {
        auto h = _System::scale_factors(_point);

        value_type dA = static_cast<value_type>(1);

        for (std::size_t i = 0; i < dim; ++i)
        {
            if (i != _omit_axis)
            {
                dA *= h[i];
            }
        }

        return dA;
    }

    // gradient_component
    //   returns the _I-th component of the gradient of a scalar field.
    // (∇f)_i = (1/h_i)(∂f/∂q_i).
    // The caller provides the partial derivative value.
    static value_type
    gradient_component
    (
        const point_type& _point,
        std::size_t       _axis,
        value_type        _partial_derivative
    ) noexcept
    {
        auto h = _System::scale_factors(_point);

        if (h[_axis] == static_cast<value_type>(0))
        {
            return static_cast<value_type>(0);
        }

        return _partial_derivative / h[_axis];
    }

    // divergence_prefactor
    //   returns the prefactor (1/J) for computing the divergence.
    // div(F) = (1/J) Σ ∂/∂q_i (J/h_i · F_i)
    // where J = ∏ h_j is the Jacobian.
    static value_type
    divergence_prefactor
    (
        const point_type& _point
    ) noexcept
    {
        value_type j = _System::jacobian(_point);

        if (j == static_cast<value_type>(0))
        {
            return static_cast<value_type>(0);
        }

        return static_cast<value_type>(1) / j;
    }

    // laplacian_scale
    //   returns the scale factors needed for the Laplacian.
    // ∇²f = (1/J) Σ ∂/∂q_i ((J/h_i²)(∂f/∂q_i))
    // Returns array of J/h_i² for each axis.
    static std::array<value_type, dim>
    laplacian_scale
    (
        const point_type& _point
    ) noexcept
    {
        auto h = _System::scale_factors(_point);
        value_type j = _System::jacobian(_point);

        std::array<value_type, dim> scale{};

        for (std::size_t i = 0; i < dim; ++i)
        {
            value_type hi2 = h[i] * h[i];

            if (hi2 == static_cast<value_type>(0))
            {
                scale[i] = static_cast<value_type>(0);
            }
            else
            {
                scale[i] = j / hi2;
            }
        }

        return scale;
    }
};


// ============================================================================
// IV.   COORDINATE SYSTEM DETECTION HELPERS
// ============================================================================

NS_INTERNAL

    // is_coordinate_system_structural_check
    //   helper: checks if type satisfies the coordinate system
    // structural interface.
    template<typename _Type,
             typename = void>
    struct is_coord_system_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_coord_system_check<_Type, std::enable_if_t<
        ( std::is_same<decltype(_Type::dimension),
                       const std::size_t>::value          &&
          std::is_same<decltype(_Type::is_orthogonal),
                       const bool>::value )
    >> : std::true_type
    {
    };

    // same_system_check
    //   helper: checks if two coordinate systems are the same type.
    template<typename _A,
             typename _B,
             typename = void>
    struct same_system_check : std::false_type
    {
    };

    template<typename _A,
             typename _B>
    struct same_system_check<_A, _B, std::enable_if_t<
        std::is_same<_A, _B>::value
    >> : std::true_type
    {
    };

    // compatible_systems_check
    //   helper: checks if two coordinate systems have the same
    // dimension and can be converted between each other.
    template<typename _A,
             typename _B,
             typename = void>
    struct compatible_systems_check : std::false_type
    {
    };

    template<typename _A,
             typename _B>
    struct compatible_systems_check<_A, _B, std::enable_if_t<
        ( is_coord_system_check<_A>::value &&
          is_coord_system_check<_B>::value &&
          (_A::dimension == _B::dimension) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_coord_system
//   trait: checks if _Type is a coordinate system.
template<typename _Type>
struct is_coord_system : internal::is_coord_system_check<_Type>
{
};

// are_same_system
//   trait: checks if two coordinate systems are the same.
template<typename _A,
         typename _B>
struct are_same_system : internal::same_system_check<_A, _B>
{
};

// are_compatible_systems
//   trait: checks if two coordinate systems have matching dimension.
template<typename _A,
         typename _B>
struct are_compatible_systems
    : internal::compatible_systems_check<_A, _B>
{
};


// ============================================================================
// V.    VARIABLE TEMPLATES
// ============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_coord_system_v
    //   variable template: value helper for is_coord_system.
    template<typename _Type>
    inline constexpr bool is_coord_system_v =
        is_coord_system<_Type>::value;

    // are_same_system_v
    //   variable template: value helper for are_same_system.
    template<typename _A,
             typename _B>
    inline constexpr bool are_same_system_v =
        are_same_system<_A, _B>::value;

    // are_compatible_systems_v
    //   variable template: value helper for are_compatible_systems.
    template<typename _A,
             typename _B>
    inline constexpr bool are_compatible_systems_v =
        are_compatible_systems<_A, _B>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ============================================================================
// VI.   TYPE ALIASES
// ============================================================================

// --- convenience point types -------------------------------------------------

// point_2d
//   type: 2D Cartesian point.
template<typename _T = double>
using point_2d = cartesian_point<2, _T>;

// point_3d
//   type: 3D Cartesian point.
template<typename _T = double>
using point_3d = cartesian_point<3, _T>;


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_COORDINATE_
