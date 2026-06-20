/******************************************************************************
* djinterp [math]                                         geometry_common.hpp
*
* Foundational geometry infrastructure shared by 2D and 3D modules.
*   Provides the structural interface documentation for edge and shape
* types, SFINAE detection traits, coordinate-system-aware lerp/distance
* helpers, axis-aligned bounding boxes, and composite Simpson's rule for
* numerical integration. Nothing in this header is dimension-specific;
* both shape_2d / measure_2d and future 3D modules build on it.
*
* STRUCTURAL EDGE INTERFACE (compile-time detectable):
*   - using coordinate_system, value_type, point_type
*   - static constexpr std::size_t dimension
*   - static constexpr bool is_edge   = true
*   - static constexpr bool is_curved
*   - static constexpr bool is_closed
*   - value_type parameter_min() const
*   - value_type parameter_max() const
*   - point_type at(value_type _t) const
*   - point_type start() const
*   - point_type end() const
*   - (optional) value_type closed_form_length() const
*
* STRUCTURAL SHAPE INTERFACE (compile-time detectable):
*   - using coordinate_system, value_type, point_type
*   - static constexpr std::size_t dimension
*   - static constexpr bool is_shape = true
*   - static constexpr bool is_2d
*   - static constexpr bool is_3d
*   - bool contains(const point_type& _p) const
*   - (optional) value_type closed_form_perimeter() const
*   - (optional) value_type closed_form_area() const
*   - (optional) value_type closed_form_volume() const           [3D]
*   - (optional) point_type closed_form_centroid() const
*
* path:      /inc/djinterp/math/geometry/geometry_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_COMMON_
#define DJINTERP_MATH_GEOMETRY_COMMON_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <limits>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../math.hpp"
#include "../coordinate.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    STRUCTURAL DETECTION TRAITS
// ============================================================================

NS_INTERNAL

    // has_is_edge_check
    //   helper: detects whether _T declares `static constexpr bool is_edge`
    // (and that flag is true).
    template<typename _T,
             typename = void>
    struct has_is_edge_check : std::false_type
    {
    };

    template<typename _T>
    struct has_is_edge_check<_T, std::void_t<
        decltype(_T::is_edge)
    >> : std::integral_constant<bool, _T::is_edge>
    {
    };

    // has_is_shape_check
    //   helper: detects whether _T declares `static constexpr bool is_shape`
    // (and that flag is true).
    template<typename _T,
             typename = void>
    struct has_is_shape_check : std::false_type
    {
    };

    template<typename _T>
    struct has_is_shape_check<_T, std::void_t<
        decltype(_T::is_shape)
    >> : std::integral_constant<bool, _T::is_shape>
    {
    };

    // has_is_surface_check
    //   helper: detects whether _T declares `static constexpr bool
    // is_surface` (and that flag is true).
    template<typename _T,
             typename = void>
    struct has_is_surface_check : std::false_type
    {
    };

    template<typename _T>
    struct has_is_surface_check<_T, std::void_t<
        decltype(_T::is_surface)
    >> : std::integral_constant<bool, _T::is_surface>
    {
    };

    // has_closed_form_surface_area_check
    //   helper: detects `closed_form_surface_area()` on a const instance
    // of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_surface_area_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_surface_area_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_surface_area())
    >> : std::true_type
    {
    };

    // has_closed_form_length_check
    //   helper: detects `closed_form_length()` on a const instance of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_length_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_length_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_length())
    >> : std::true_type
    {
    };

    // has_closed_form_perimeter_check
    //   helper: detects `closed_form_perimeter()` on a const instance of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_perimeter_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_perimeter_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_perimeter())
    >> : std::true_type
    {
    };

    // has_closed_form_area_check
    //   helper: detects `closed_form_area()` on a const instance of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_area_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_area_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_area())
    >> : std::true_type
    {
    };

    // has_closed_form_volume_check
    //   helper: detects `closed_form_volume()` on a const instance of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_volume_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_volume_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_volume())
    >> : std::true_type
    {
    };

    // has_closed_form_centroid_check
    //   helper: detects `closed_form_centroid()` on a const instance of _T.
    template<typename _T,
             typename = void>
    struct has_closed_form_centroid_check : std::false_type
    {
    };

    template<typename _T>
    struct has_closed_form_centroid_check<_T, std::void_t<
        decltype(std::declval<const _T&>().closed_form_centroid())
    >> : std::true_type
    {
    };

NS_END  // internal

// is_edge
//   trait: true if _T satisfies the structural edge interface.
template<typename _T>
struct is_edge : internal::has_is_edge_check<_T>
{
};

// is_shape
//   trait: true if _T satisfies the structural shape interface.
template<typename _T>
struct is_shape : internal::has_is_shape_check<_T>
{
};

// is_surface
//   trait: true if _T satisfies the structural surface interface.
template<typename _T>
struct is_surface : internal::has_is_surface_check<_T>
{
};

// has_closed_form_surface_area
//   trait: true if _T provides closed_form_surface_area().
template<typename _T>
struct has_closed_form_surface_area
    : internal::has_closed_form_surface_area_check<_T>
{
};

// has_closed_form_length
//   trait: true if _T provides closed_form_length().
template<typename _T>
struct has_closed_form_length
    : internal::has_closed_form_length_check<_T>
{
};

// has_closed_form_perimeter
//   trait: true if _T provides closed_form_perimeter().
template<typename _T>
struct has_closed_form_perimeter
    : internal::has_closed_form_perimeter_check<_T>
{
};

// has_closed_form_area
//   trait: true if _T provides closed_form_area().
template<typename _T>
struct has_closed_form_area
    : internal::has_closed_form_area_check<_T>
{
};

// has_closed_form_volume
//   trait: true if _T provides closed_form_volume().
template<typename _T>
struct has_closed_form_volume
    : internal::has_closed_form_volume_check<_T>
{
};

// has_closed_form_centroid
//   trait: true if _T provides closed_form_centroid().
template<typename _T>
struct has_closed_form_centroid
    : internal::has_closed_form_centroid_check<_T>
{
};


// ============================================================================
// II.   CARTESIAN PIVOT HELPERS
// ============================================================================
// Many geometry operations (linear interpolation, Euclidean distance,
// signed area) have a clean meaning in Cartesian space and a much messier
// one in curvilinear systems. These helpers route through Cartesian using
// the coordinate system's own to_cartesian / from_cartesian transforms,
// keeping the rest of the geometry code agnostic to which system the
// user picked.

// cartesian_lerp
//   linearly interpolates between two points expressed in any coordinate
// system. Interpolation is performed in Cartesian space; the result is
// converted back into the source coordinate system.
template<typename _System>
typename _System::point_type
cartesian_lerp
(
    const typename _System::point_type& _a,
    const typename _System::point_type& _b,
    typename _System::value_type        _t
) noexcept
{
    using value_type = typename _System::value_type;
    constexpr std::size_t dim = _System::dimension;

    auto ca = _System::to_cartesian(_a);
    auto cb = _System::to_cartesian(_b);

    std::array<value_type, dim> p{};

    // interpolate component-wise in cartesian space
    for (std::size_t i = 0; i < dim; ++i)
    {
        p[i] = ca[i] + _t * (cb[i] - ca[i]);
    }

    return _System::from_cartesian(p);
}

// cartesian_distance
//   computes Euclidean distance between two points in any coordinate
// system by converting both to Cartesian first.
template<typename _System>
typename _System::value_type
cartesian_distance
(
    const typename _System::point_type& _a,
    const typename _System::point_type& _b
) noexcept
{
    using value_type = typename _System::value_type;
    constexpr std::size_t dim = _System::dimension;

    auto ca = _System::to_cartesian(_a);
    auto cb = _System::to_cartesian(_b);

    value_type sum = static_cast<value_type>(0);

    // sum squared component differences
    for (std::size_t i = 0; i < dim; ++i)
    {
        value_type d = cb[i] - ca[i];
        sum += d * d;
    }

    return std::sqrt(sum);
}

// cartesian_dot
//   dot product of two equal-length Cartesian-space vectors stored as
// raw std::arrays. Used internally by 3D measurement code where the
// inputs are already in Cartesian; coordinate-system wrappers can call
// this after their own to_cartesian() conversion.
template<typename    _T,
         std::size_t _N>
constexpr _T
cartesian_dot
(
    const std::array<_T, _N>& _a,
    const std::array<_T, _N>& _b
) noexcept
{
    _T sum = static_cast<_T>(0);

    // straight dot accumulation
    for (std::size_t i = 0; i < _N; ++i)
    {
        sum += _a[i] * _b[i];
    }

    return sum;
}

// cartesian_cross_3
//   3D cross product of two Cartesian-space vectors. Returns
// a × b. Only defined for 3-element arrays; higher-dimensional
// generalisations (wedge product, etc.) are out of scope here.
template<typename _T>
constexpr std::array<_T, 3>
cartesian_cross_3
(
    const std::array<_T, 3>& _a,
    const std::array<_T, 3>& _b
) noexcept
{
    return {{ _a[1] * _b[2] - _a[2] * _b[1],
              _a[2] * _b[0] - _a[0] * _b[2],
              _a[0] * _b[1] - _a[1] * _b[0] }};
}

// cartesian_sub_3
//   component-wise subtraction of two 3D Cartesian vectors. A small
// convenience for code that needs (b - a) as an edge vector before
// taking a cross product.
template<typename _T>
constexpr std::array<_T, 3>
cartesian_sub_3
(
    const std::array<_T, 3>& _a,
    const std::array<_T, 3>& _b
) noexcept
{
    return {{ _a[0] - _b[0],
              _a[1] - _b[1],
              _a[2] - _b[2] }};
}


// ============================================================================
// III.  AXIS-ALIGNED BOUNDING BOX
// ============================================================================

// aabb
//   struct: axis-aligned bounding box in Cartesian space. Stores the
// minimum and maximum corners. Dimension-agnostic so the same template
// serves 2D and 3D.
template<std::size_t _Dim,
         typename    _ValueType = double>
struct aabb
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _ValueType;
    using point_type = std::array<value_type, _Dim>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = _Dim;

    // ---- data ---------------------------------------------------------------

    point_type m_min;
    point_type m_max;

    // ---- construction -------------------------------------------------------

    constexpr aabb() noexcept
        : m_min{},
          m_max{}
    {
    }

    constexpr aabb(
        const point_type& _min_corner,
        const point_type& _max_corner
    ) noexcept
        : m_min(_min_corner),
          m_max(_max_corner)
    {
    }

    // ---- queries ------------------------------------------------------------

    // extent
    //   returns the length of the box along the given axis.
    constexpr value_type
    extent
    (
        std::size_t _axis
    ) const noexcept
    {
        return m_max[_axis] - m_min[_axis];
    }

    // center
    //   returns the midpoint of the box in Cartesian space.
    constexpr point_type
    center
    () const noexcept
    {
        point_type c{};

        for (std::size_t i = 0; i < _Dim; ++i)
        {
            c[i] = (m_min[i] + m_max[i]) /
                   static_cast<value_type>(2);
        }

        return c;
    }

    // contains
    //   tests whether a Cartesian point lies within the box.
    constexpr bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        // any axis out of range fails the test
        for (std::size_t i = 0; i < _Dim; ++i)
        {
            if ( (_p[i] < m_min[i]) ||
                 (_p[i] > m_max[i]) )
            {
                return false;
            }
        }

        return true;
    }

    // ---- mutation -----------------------------------------------------------

    // include
    //   expands the box to contain the given Cartesian point.
    void
    include
    (
        const point_type& _p
    ) noexcept
    {
        for (std::size_t i = 0; i < _Dim; ++i)
        {
            if (_p[i] < m_min[i])
            {
                m_min[i] = _p[i];
            }

            if (_p[i] > m_max[i])
            {
                m_max[i] = _p[i];
            }
        }

        return;
    }

    // ---- factories ----------------------------------------------------------

    // empty
    //   returns an empty box, suitable for incremental construction.
    // Min is +inf on every axis, max is -inf; any include() call
    // immediately tightens both corners.
    static aabb
    empty
    () noexcept
    {
        point_type lo{};
        point_type hi{};

        // initialise to extremes so any include() shrinks them
        for (std::size_t i = 0; i < _Dim; ++i)
        {
            lo[i] =  std::numeric_limits<value_type>::infinity();
            hi[i] = -std::numeric_limits<value_type>::infinity();
        }

        return aabb{lo, hi};
    }
};


// ============================================================================
// IV.   NUMERICAL QUADRATURE (composite Simpson's rule)
// ============================================================================
// One-dimensional numerical integration used as the fallback for
// measurement when no closed-form expression is available (e.g. arc
// length of a general parametric edge). Higher-order quadrature
// (Gauss-Legendre) is reserved for a future revision.

NS_INTERNAL

    // simpson_composite
    //   helper: composite Simpson's rule for ∫_a^b f(t) dt with _n
    // subintervals. _n is rounded up to the next even integer (Simpson's
    // requires an even subinterval count).
    template<typename _Func,
             typename _ValueType>
    _ValueType
    simpson_composite
    (
        _Func        _f,
        _ValueType   _a,
        _ValueType   _b,
        std::size_t  _n
    ) noexcept
    {
        _ValueType  h;
        _ValueType  total;
        std::size_t n;

        n = _n;

        // ensure n is at least 2 and even
        if (n < 2)
        {
            n = 2;
        }

        if ((n % 2) != 0)
        {
            ++n;
        }

        h     = (_b - _a) / static_cast<_ValueType>(n);
        total = _f(_a) + _f(_b);

        // weight pattern: 4, 2, 4, 2, ..., 4 for interior points
        for (std::size_t i = 1; i < n; ++i)
        {
            _ValueType t = _a +
                static_cast<_ValueType>(i) * h;

            _ValueType w = ((i % 2) == 0)
                ? static_cast<_ValueType>(2)
                : static_cast<_ValueType>(4);

            total += w * _f(t);
        }

        return total * h / static_cast<_ValueType>(3);
    }

NS_END  // internal

// integrate_simpson
//   numerically integrates _f over the closed interval [_a, _b] using
// composite Simpson's rule. _n subintervals (rounded up to an even
// count if odd). Used by curve_length and area when the shape provides
// no closed-form expression.
template<typename _Func,
         typename _ValueType>
_ValueType
integrate_simpson
(
    _Func        _f,
    _ValueType   _a,
    _ValueType   _b,
    std::size_t  _n = 64
) noexcept
{
    return internal::simpson_composite(_f, _a, _b, _n);
}


// ============================================================================
// V.    VARIABLE TEMPLATES
// ============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_edge_v
    //   variable template: value helper for is_edge.
    template<typename _T>
    inline constexpr bool is_edge_v = is_edge<_T>::value;

    // is_shape_v
    //   variable template: value helper for is_shape.
    template<typename _T>
    inline constexpr bool is_shape_v = is_shape<_T>::value;

    // is_surface_v
    //   variable template: value helper for is_surface.
    template<typename _T>
    inline constexpr bool is_surface_v = is_surface<_T>::value;

    // has_closed_form_surface_area_v
    template<typename _T>
    inline constexpr bool has_closed_form_surface_area_v =
        has_closed_form_surface_area<_T>::value;

    // has_closed_form_length_v
    template<typename _T>
    inline constexpr bool has_closed_form_length_v =
        has_closed_form_length<_T>::value;

    // has_closed_form_perimeter_v
    template<typename _T>
    inline constexpr bool has_closed_form_perimeter_v =
        has_closed_form_perimeter<_T>::value;

    // has_closed_form_area_v
    template<typename _T>
    inline constexpr bool has_closed_form_area_v =
        has_closed_form_area<_T>::value;

    // has_closed_form_volume_v
    template<typename _T>
    inline constexpr bool has_closed_form_volume_v =
        has_closed_form_volume<_T>::value;

    // has_closed_form_centroid_v
    template<typename _T>
    inline constexpr bool has_closed_form_centroid_v =
        has_closed_form_centroid<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// ============================================================================
// VI.   COMMON CONSTANTS
// ============================================================================

// pi_v
//   constant: the mathematical constant π, typed to _ValueType.
template<typename _ValueType>
constexpr _ValueType pi_v =
    static_cast<_ValueType>(3.14159265358979323846L);

// two_pi_v
//   constant: 2π, typed to _ValueType.
template<typename _ValueType>
constexpr _ValueType two_pi_v =
    static_cast<_ValueType>(2) * pi_v<_ValueType>;

// half_pi_v
//   constant: π/2, typed to _ValueType.
template<typename _ValueType>
constexpr _ValueType half_pi_v =
    pi_v<_ValueType> / static_cast<_ValueType>(2);


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_COMMON_
