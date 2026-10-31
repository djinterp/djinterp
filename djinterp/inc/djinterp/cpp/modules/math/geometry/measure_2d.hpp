/*******************************************************************************
* djinterp [cpp/math]                                             measure_2d.hpp
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
*   area_green<Edge>(e)           - area a closed edge bounds (Green)
*   centroid<Shape>(s)            - geometric centroid
*   bounding_box_of_edge, _of_polygon, _of_circle
*                                 - axis-aligned bbox in Cartesian
*
* LEVELS:
*   Everything compiles from C++98: the closed-form dispatch is a bool
* template parameter where it was an enable_if_t default, and the
* integrands are function objects where they were lambdas, the formulas
* and their order unchanged. The header now includes named_2d.hpp, whose
* circle bounding_box_of_circle measures; without it the header compiled
* at no level.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/measure_2d.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LENGTHS
    -------
    1.  curve_length
    2.  perimeter
2.  AREAS AND CENTROIDS
    -------------------
    1.  area
    2.  area_green
    3.  centroid
3.  BOUNDING BOXES
    --------------
*/

#ifndef DJINTERP_MATH_GEOMETRY_MEASURE_2D_HPP
#define DJINTERP_MATH_GEOMETRY_MEASURE_2D_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "./edge.hpp"                             // the edges
#include "./geometry_common.hpp"                  // the traits, aabb,
                                                  // integrate_simpson
#include "./named_2d.hpp"                         // circle
#include "./shape_2d.hpp"                         // polygon


NS_DJINTERP
NS_MATH


NS_INTERNAL

    // edge_speed
    //   struct: |d/dt at(t)|, by central differences of half-width _eps:
    // the integrand of an edge's arc length.
    template<typename Edge>
    struct edge_speed
    {
        typedef typename Edge::value_type        value_type;
        typedef typename Edge::coordinate_system system_type;

        const Edge& m_edge;
        value_type  m_eps;

        edge_speed(
            const Edge& _edge,
            value_type  _eps
        )
            : m_edge(_edge),
              m_eps(_eps)
        {}

        value_type operator()(
            value_type _t
        ) const
        {
            return cartesian_distance<system_type>(m_edge.at(_t - m_eps),
                                                   m_edge.at(_t + m_eps)) /
                   (static_cast<value_type>(2) * m_eps);
        }
    };

    // green_integrand
    //   struct: x y' - y x', central differences of half-width _eps: the
    // integrand of Green's area.
    template<typename Edge>
    struct green_integrand
    {
        typedef typename Edge::value_type        value_type;
        typedef typename Edge::coordinate_system system_type;
        typedef typename Edge::point_type        point_type;

        const Edge& m_edge;
        value_type  m_eps;

        green_integrand(
            const Edge& _edge,
            value_type  _eps
        )
            : m_edge(_edge),
              m_eps(_eps)
        {}

        value_type operator()(
            value_type _t
        ) const
        {
            const point_type p_cart  = system_type::to_cartesian(
                                           m_edge.at(_t));
            const point_type p_fwd   = system_type::to_cartesian(
                                           m_edge.at(_t + m_eps));
            const point_type p_bck   = system_type::to_cartesian(
                                           m_edge.at(_t - m_eps));
            const value_type two_eps = static_cast<value_type>(2) * m_eps;
            const value_type dx_dt   = (p_fwd[0] - p_bck[0]) / two_eps;
            const value_type dy_dt   = (p_fwd[1] - p_bck[1]) / two_eps;

            return (p_cart[0] * dy_dt) - (p_cart[1] * dx_dt);
        }
    };

    // curve_length_impl
    //   struct: an edge's closed-form length where it has one, Simpson's
    // rule over its speed otherwise.
    template<typename Edge,
             bool     Closed = has_closed_form_length<Edge>::value>
    struct curve_length_impl
    {
        typedef typename Edge::value_type value_type;

        static value_type
        compute(
            const Edge& _e,
            std::size_t _samples
        )
        {
            const value_type t_min = _e.parameter_min();
            const value_type t_max = _e.parameter_max();
            const value_type eps   = (t_max - t_min) /
                                     static_cast<value_type>(_samples * 64);

            return integrate_simpson(edge_speed<Edge>(_e, eps),
                                     t_min,
                                     t_max,
                                     _samples);
        }
    };

    template<typename Edge>
    struct curve_length_impl<Edge, true>
    {
        typedef typename Edge::value_type value_type;

        static
        value_type compute(
            const Edge& _e,
            std::size_t
        )
        {
            return _e.closed_form_length();
        }
    };

    // measure_edge_check, measure_edge_2d_check, measure_shape_check,
    // measure_shape_2d_check
    //   struct: the measurements' requirements, asserted at class scope,
    // once for every function that names them.
    template<typename Edge>
    struct measure_edge_check
    {
        D_STATIC_ASSERT((is_edge<Edge>::value),
                        "measure_2d: Edge must satisfy the edge interface.");

        enum
        {
            ok = 1
        };
    };

    template<typename Edge>
    struct measure_edge_2d_check
    {
        D_STATIC_ASSERT((measure_edge_check<Edge>::ok == 1) &&
                        (Edge::dimension == 2),
                        "measure_2d: 2D edges only.");

        enum
        {
            ok = 1
        };
    };

    template<typename Shape>
    struct measure_shape_check
    {
        D_STATIC_ASSERT((is_shape<Shape>::value),
                        "measure_2d: Shape must satisfy the shape "
                        "interface.");

        enum
        {
            ok = 1
        };
    };

    template<typename Shape>
    struct measure_shape_2d_check
    {
        D_STATIC_ASSERT((measure_shape_check<Shape>::ok == 1) &&
                        (Shape::is_2d),
                        "measure_2d: 2D shapes only (see measure_3d for "
                        "surface area of 3D shapes).");

        enum
        {
            ok = 1
        };
    };

    // perimeter_impl, area_impl, centroid_impl
    //   struct: the shape's closed form; with none, a compile-time error
    // saying what to provide.
    template<typename Shape,
             bool     Closed = has_closed_form_perimeter<Shape>::value>
    struct perimeter_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "perimeter: no closed-form perimeter and no "
                        "numerical fallback applies to this shape. "
                        "Provide closed_form_perimeter() or measure "
                        "each edge with curve_length() and sum.");
    };

    template<typename Shape>
    struct perimeter_impl<Shape, true>
    {
        static
        typename Shape::value_type compute(
            const Shape& _s
        )
        {
            return _s.closed_form_perimeter();
        }
    };

    template<typename Shape,
             bool     Closed = has_closed_form_area<Shape>::value>
    struct area_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "area: no closed-form area available. Provide "
                        "closed_form_area() on the shape, or call "
                        "area_green() with an explicit boundary edge.");
    };

    template<typename Shape>
    struct area_impl<Shape, true>
    {
        static
        typename Shape::value_type compute(
            const Shape& _s
        )
        {
            return _s.closed_form_area();
        }
    };

    template<typename Shape,
             bool     Closed = has_closed_form_centroid<Shape>::value>
    struct centroid_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "centroid: no closed_form_centroid() and no "
                        "general fallback. Define closed_form_centroid "
                        "on the shape.");
    };

    template<typename Shape>
    struct centroid_impl<Shape, true>
    {
        static
        typename Shape::point_type compute(
            const Shape& _s
        )
        {
            return _s.closed_form_centroid();
        }
    };

NS_END  // internal


//==============================================================================
// 1.  LENGTHS
//==============================================================================


// 1.1    curve_length
//------------------------------------------------------------------------------
// 1.1.1
// curve_length
//   function: an edge's arc length -- its closed form, or Simpson's rule
// over _samples intervals of its speed.
template<typename Edge>
typename Edge::value_type curve_length(
    const Edge& _e,
    std::size_t _samples = 64
)
{
    (void)internal::measure_edge_check<Edge>::ok;

    return internal::curve_length_impl<Edge>::compute(_e, _samples);
}

// 1.2    perimeter
//------------------------------------------------------------------------------
// 1.2.1
// perimeter
//   function: a 2D shape's boundary length, its closed form.
template<typename Shape>
typename Shape::value_type perimeter(
    const Shape& _s
)
{
    (void)internal::measure_shape_2d_check<Shape>::ok;

    return internal::perimeter_impl<Shape>::compute(_s);
}


//==============================================================================
// 2.  AREAS AND CENTROIDS
//==============================================================================


// 2.1    area
//------------------------------------------------------------------------------
// 2.1.1
// area
//   function: a 2D shape's area, its closed form.
template<typename Shape>
typename Shape::value_type area(
    const Shape& _s
)
{
    (void)internal::measure_shape_2d_check<Shape>::ok;

    return internal::area_impl<Shape>::compute(_s);
}

// 2.2    area_green
//------------------------------------------------------------------------------
// 2.2.1
// area_green
//   function: the area a closed 2D edge bounds, |∮ (x dy - y dx)| / 2 by
// Simpson's rule over _samples intervals.
template<typename Edge>
typename Edge::value_type area_green(
    const Edge& _boundary,
    std::size_t _samples = 256
)
{
    (void)internal::measure_edge_2d_check<Edge>::ok;

    typedef typename Edge::value_type value_type;

    const value_type t_min  = _boundary.parameter_min();
    const value_type t_max  = _boundary.parameter_max();
    const value_type eps    = (t_max - t_min) /
                              static_cast<value_type>(_samples * 64);
    const value_type result = integrate_simpson(
                                  internal::green_integrand<Edge>(_boundary,
                                                                  eps),
                                  t_min,
                                  t_max,
                                  _samples) /
                              static_cast<value_type>(2);

    return (result < static_cast<value_type>(0)) ? -result : result;
}

// 2.3    centroid
//------------------------------------------------------------------------------
// 2.3.1
// centroid
//   function: a shape's centroid, its closed form.
template<typename Shape>
typename Shape::point_type centroid(
    const Shape& _s
)
{
    (void)internal::measure_shape_check<Shape>::ok;

    return internal::centroid_impl<Shape>::compute(_s);
}


//==============================================================================
// 3.  BOUNDING BOXES
//==============================================================================


// bounding_box_of_edge
//   function: the Cartesian box of _samples + 1 points along a 2D edge.
template<typename Edge>
aabb<2, typename Edge::value_type> bounding_box_of_edge(
    const Edge& _e,
    std::size_t _samples = 64
)
{
    (void)internal::measure_edge_2d_check<Edge>::ok;

    typedef typename Edge::value_type        value_type;
    typedef typename Edge::coordinate_system system_type;

    const value_type    t_min = _e.parameter_min();
    const value_type    t_max = _e.parameter_max();
    const value_type    step  = (t_max - t_min) /
                                static_cast<value_type>(_samples);
    aabb<2, value_type> box   = aabb<2, value_type>::empty();

    for (std::size_t i = 0; i <= _samples; ++i)
    {
        box.include(system_type::to_cartesian(
            _e.at(t_min + (static_cast<value_type>(i) * step))));
    }

    return box;
}

// bounding_box_of_polygon
//   function: the Cartesian box of a polygon's vertices.
template<typename    System,
         std::size_t N>
aabb<2, typename System::value_type> bounding_box_of_polygon(
    const polygon<System, N>& _poly
)
{
    typedef typename System::value_type value_type;

    aabb<2, value_type> box = aabb<2, value_type>::empty();

    for (std::size_t i = 0; i < N; ++i)
    {
        box.include(System::to_cartesian(_poly.m_vertices[i]));
    }

    return box;
}

// bounding_box_of_circle
//   function: a circle's Cartesian box, its centre plus and minus its
// radius.
template<typename System>
aabb<2, typename System::value_type> bounding_box_of_circle(
    const circle<System>& _c
)
{
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    const point_type cc = System::to_cartesian(_c.m_center);

    return aabb<2, value_type>(point_type(cc[0] - _c.m_radius,
                                          cc[1] - _c.m_radius),
                               point_type(cc[0] + _c.m_radius,
                                          cc[1] + _c.m_radius));
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_MEASURE_2D_HPP
