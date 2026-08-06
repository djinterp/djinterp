/******************************************************************************
* djinterp [math]                                               measure_2d.hpp
*
* 2D measurements with closed-form / numerical dispatch.
*   Provides free function templates that compute geometric quantities
* of edges and shapes. Each measurement prefers a closed-form method
* on the operand when one is present (detected structurally via the
* has_closed_form_* traits from geometry_common.hpp); when no closed
* form is available, it falls back to a numerical procedure built on
* composite Simpson's rule.
*
* PROVIDED FUNCTIONS:
*   curve_length<Edge>(e)         - arc length of an edge
*   perimeter<Shape>(s)           - boundary length of a 2D shape
*   area<Shape>(s)                - signed-absolute area of a 2D shape
*   centroid<Shape>(s)            - geometric centroid
*   bounding_box<Edge|Shape>(x)   - axis-aligned bbox in Cartesian
*
* path:      /inc/djinterp/math/geometry/measure_2d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_MEASURE_2D_
#define DJINTERP_MATH_GEOMETRY_MEASURE_2D_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"
#include "./edge.hpp"
#include "./shape_2d.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    CURVE LENGTH (per-edge)
// ============================================================================

NS_INTERNAL

    // curve_length_impl (general fallback)
    //   helper: arc length by numerical integration of |dr/dt|
    // approximated by central difference.
    template<typename _Edge,
             typename = void>
    struct curve_length_impl
    {
        using value_type = typename _Edge::value_type;

        static value_type
        compute
        (
            const _Edge& _e,
            std::size_t  _samples
        ) noexcept
        {
            value_type t_min;
            value_type t_max;
            value_type eps;

            t_min = _e.parameter_min();
            t_max = _e.parameter_max();

            // central-difference step proportional to interval
            eps = (t_max - t_min) /
                  static_cast<value_type>(_samples * 64);

            // speed = |dr/dt| ≈ d(p1, p2) / (2·eps), Cartesian
            auto speed = [&_e, eps](value_type _t) -> value_type
            {
                using sys = typename _Edge::coordinate_system;

                auto p1 = _e.at(_t - eps);
                auto p2 = _e.at(_t + eps);

                return cartesian_distance<sys>(p1, p2) /
                       (static_cast<value_type>(2) * eps);
            };

            return integrate_simpson<decltype(speed),
                                     value_type>(speed,
                                                  t_min,
                                                  t_max,
                                                  _samples);
        }
    };

    // curve_length_impl (closed-form specialisation)
    //   used when _Edge provides closed_form_length().
    template<typename _Edge>
    struct curve_length_impl<_Edge, std::enable_if_t<
        has_closed_form_length<_Edge>::value
    >>
    {
        using value_type = typename _Edge::value_type;

        static value_type
        compute
        (
            const _Edge& _e,
            std::size_t
        ) noexcept
        {
            return _e.closed_form_length();
        }
    };

NS_END  // internal

// curve_length
//   computes the arc length of an edge. Uses closed_form_length when
// available, otherwise numerical integration with _samples Simpson
// subintervals.
template<typename _Edge>
typename _Edge::value_type
curve_length
(
    const _Edge& _e,
    std::size_t  _samples = 64
) noexcept
{
    static_assert(is_edge<_Edge>::value,
                  "curve_length: _Edge must satisfy the edge "
                  "interface.");

    return internal::curve_length_impl<_Edge>::compute(_e, _samples);
}


// ============================================================================
// II.   PERIMETER (per-shape)
// ============================================================================

NS_INTERNAL

    // perimeter_impl (general fallback)
    //   helper: declared only; specialisations cover the cases we
    // can actually compute. The unspecialised primary deliberately
    // triggers a static_assert via a sentinel.
    template<typename _Shape,
             typename = void>
    struct perimeter_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "perimeter: no closed-form perimeter and no "
                      "numerical fallback applies to this shape. "
                      "Provide closed_form_perimeter() or measure "
                      "each edge with curve_length() and sum.");
    };

    // perimeter_impl (closed-form specialisation)
    template<typename _Shape>
    struct perimeter_impl<_Shape, std::enable_if_t<
        has_closed_form_perimeter<_Shape>::value
    >>
    {
        using value_type = typename _Shape::value_type;

        static value_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_perimeter();
        }
    };

NS_END  // internal

// perimeter
//   computes the perimeter (total boundary length) of a 2D shape.
// Currently dispatches to closed_form_perimeter when present; users
// of shape_from_edges should sum curve_length over the edge tuple.
template<typename _Shape>
typename _Shape::value_type
perimeter
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "perimeter: _Shape must satisfy the shape "
                  "interface.");
    static_assert(_Shape::is_2d,
                  "perimeter: 2D shapes only (see measure_3d "
                  "for surface area of 3D shapes).");

    return internal::perimeter_impl<_Shape>::compute(_s);
}


// ============================================================================
// III.  AREA (per-shape)
// ============================================================================

NS_INTERNAL

    // area_impl (general fallback)
    //   helper: declared only; primary triggers a static_assert when
    // instantiated. We deliberately do NOT auto-numerical here -
    // numerical area requires the user to opt into a sampling
    // strategy (Green's theorem on a closed boundary) and choose a
    // sample count.
    template<typename _Shape,
             typename = void>
    struct area_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "area: no closed-form area available. Provide "
                      "closed_form_area() on the shape, or call "
                      "area_green() with an explicit boundary edge.");
    };

    // area_impl (closed-form specialisation)
    template<typename _Shape>
    struct area_impl<_Shape, std::enable_if_t<
        has_closed_form_area<_Shape>::value
    >>
    {
        using value_type = typename _Shape::value_type;

        static value_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_area();
        }
    };

NS_END  // internal

// area
//   computes the area of a 2D shape. Closed-form only; for free-form
// shapes use area_green() below with a boundary edge.
template<typename _Shape>
typename _Shape::value_type
area
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "area: _Shape must satisfy the shape interface.");
    static_assert(_Shape::is_2d,
                  "area: 2D shapes only.");

    return internal::area_impl<_Shape>::compute(_s);
}

// area_green
//   numerically computes the planar area enclosed by a closed
// parametric edge via Green's theorem:
//   A = (1/2) ∮ (x dy - y dx)
// The user must ensure the edge is closed; the returned value is the
// absolute value of the signed integral.
template<typename _Edge>
typename _Edge::value_type
area_green
(
    const _Edge& _boundary,
    std::size_t  _samples = 256
) noexcept
{
    static_assert(is_edge<_Edge>::value,
                  "area_green: _Edge must satisfy the edge "
                  "interface.");
    static_assert(_Edge::dimension == 2,
                  "area_green: 2D edges only.");

    using value_type = typename _Edge::value_type;
    using sys        = typename _Edge::coordinate_system;

    value_type t_min;
    value_type t_max;
    value_type eps;
    value_type result;

    t_min  = _boundary.parameter_min();
    t_max  = _boundary.parameter_max();
    eps    = (t_max - t_min) /
             static_cast<value_type>(_samples * 64);

    // integrand: x · y' - y · x', evaluated in cartesian.
    // x' and y' are central differences in the parametric coord.
    auto integrand = [&_boundary, eps](value_type _t) -> value_type
    {
        auto p_cart = sys::to_cartesian(_boundary.at(_t));
        auto p_fwd  = sys::to_cartesian(_boundary.at(_t + eps));
        auto p_bck  = sys::to_cartesian(_boundary.at(_t - eps));

        value_type two_eps = static_cast<value_type>(2) * eps;
        value_type dx_dt   = (p_fwd[0] - p_bck[0]) / two_eps;
        value_type dy_dt   = (p_fwd[1] - p_bck[1]) / two_eps;

        return p_cart[0] * dy_dt - p_cart[1] * dx_dt;
    };

    result = integrate_simpson<decltype(integrand),
                                value_type>(integrand,
                                             t_min,
                                             t_max,
                                             _samples) /
             static_cast<value_type>(2);

    // report absolute area (winding orientation is user choice)
    if (result < static_cast<value_type>(0))
    {
        result = -result;
    }

    return result;
}


// ============================================================================
// IV.   CENTROID (per-shape)
// ============================================================================

NS_INTERNAL

    // centroid_impl (general fallback - declared only)
    template<typename _Shape,
             typename = void>
    struct centroid_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "centroid: no closed_form_centroid() and no "
                      "general fallback. Define closed_form_centroid "
                      "on the shape.");
    };

    // centroid_impl (closed-form specialisation)
    template<typename _Shape>
    struct centroid_impl<_Shape, std::enable_if_t<
        has_closed_form_centroid<_Shape>::value
    >>
    {
        using point_type = typename _Shape::point_type;

        static point_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_centroid();
        }
    };

NS_END  // internal

// centroid
//   geometric centroid of a 2D shape. Closed-form only.
template<typename _Shape>
typename _Shape::point_type
centroid
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "centroid: _Shape must satisfy the shape "
                  "interface.");

    return internal::centroid_impl<_Shape>::compute(_s);
}


// ============================================================================
// V.    BOUNDING BOX
// ============================================================================
// Axis-aligned bounding boxes are always reported in Cartesian space
// (the natural frame for an "axis-aligned" concept). For non-Cartesian
// shapes, the bounding box is the AABB of the shape's Cartesian image.

// bounding_box_of_edge
//   approximates the AABB of an edge by sampling _samples points
// along its parameter range and tightening the box around each
// sample. Closed-form bounding boxes for line_segment / arc /
// polyline_edge can be added later as specialisations.
template<typename _Edge>
aabb<2, typename _Edge::value_type>
bounding_box_of_edge
(
    const _Edge& _e,
    std::size_t  _samples = 64
) noexcept
{
    static_assert(is_edge<_Edge>::value,
                  "bounding_box_of_edge: _Edge must satisfy the "
                  "edge interface.");
    static_assert(_Edge::dimension == 2,
                  "bounding_box_of_edge: 2D edges only.");

    using value_type = typename _Edge::value_type;
    using sys        = typename _Edge::coordinate_system;

    value_type                t_min;
    value_type                t_max;
    value_type                step;
    aabb<2, value_type>       box;

    t_min = _e.parameter_min();
    t_max = _e.parameter_max();
    step  = (t_max - t_min) /
            static_cast<value_type>(_samples);
    box   = aabb<2, value_type>::empty();

    // include _samples + 1 boundary samples
    for (std::size_t i = 0; i <= _samples; ++i)
    {
        value_type t = t_min +
            static_cast<value_type>(i) * step;
        auto       p_cart = sys::to_cartesian(_e.at(t));

        box.include(p_cart);
    }

    return box;
}

// bounding_box_of_polygon
//   exact AABB of a polygon's vertex set.
template<typename    _System,
         std::size_t _N>
aabb<2, typename _System::value_type>
bounding_box_of_polygon
(
    const polygon<_System, _N>& _poly
) noexcept
{
    using value_type = typename _System::value_type;

    aabb<2, value_type> box;

    box = aabb<2, value_type>::empty();

    // include every vertex
    for (std::size_t i = 0; i < _N; ++i)
    {
        auto v_cart = _System::to_cartesian(_poly.m_vertices[i]);

        box.include(v_cart);
    }

    return box;
}

// bounding_box_of_circle
//   exact AABB of a circle: centre ± radius on each axis.
template<typename _System>
aabb<2, typename _System::value_type>
bounding_box_of_circle
(
    const circle<_System>& _c
) noexcept
{
    using value_type = typename _System::value_type;
    using point_2    = std::array<value_type, 2>;

    auto    cc = _System::to_cartesian(_c.m_center);
    point_2 lo;
    point_2 hi;

    lo[0] = cc[0] - _c.m_radius;
    lo[1] = cc[1] - _c.m_radius;
    hi[0] = cc[0] + _c.m_radius;
    hi[1] = cc[1] + _c.m_radius;

    return aabb<2, value_type>{lo, hi};
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_MEASURE_2D_
