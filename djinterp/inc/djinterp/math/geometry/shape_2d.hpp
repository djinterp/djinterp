/******************************************************************************
* djinterp [math]                                                  shape_2d.hpp
*
* General-purpose 2D shape composers.
*   Provides the four ways a user can describe an arbitrary planar
* shape: by vertex list (polygon), by closed parametric curve
* (parametric_region), by implicit inequality (implicit_region), or by
* an ordered tuple of heterogeneous edges (shape_from_edges). Named
* shapes (circle, rectangle, etc.) live in named_2d.hpp.
*
* Each shape satisfies the structural shape interface from
* geometry_common.hpp; measurement (area, perimeter, centroid) is
* handled separately in measure_2d.hpp.
*
* 
* path:      /inc/djinterp/math/geometry/shape_2d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_SHAPE_2D_
#define DJINTERP_MATH_GEOMETRY_SHAPE_2D_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../math.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"
#include "./edge.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    POLYGON
// ============================================================================

// polygon
//   struct: an _N-vertex planar polygon. Vertices may be specified in
// any 2D coordinate system; all internal computations (shoelace area,
// perimeter, point-in-polygon) pivot through Cartesian so the chosen
// system has no effect on numerical results - only on how the user
// expresses the input.
//
// Vertices are stored in winding order. Self-intersecting polygons
// are accepted; the shoelace formula computes signed area (orientation
// dependent), and we report its absolute value.
template<typename    _System,
         std::size_t _N>
struct polygon
{
    static_assert(_System::dimension == 2,
                  "polygon: requires a 2D coordinate system.");
    static_assert(_N >= 3,
                  "polygon: at least 3 vertices required.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension    = 2;
    static constexpr std::size_t vertex_count = _N;
    static constexpr bool        is_shape     = true;
    static constexpr bool        is_2d        = true;
    static constexpr bool        is_3d        = false;

    // ---- data ---------------------------------------------------------------

    std::array<point_type, _N> m_vertices;

    // ---- construction -------------------------------------------------------

    constexpr polygon() noexcept
        : m_vertices{}
    {
    }

    constexpr explicit polygon(
        const std::array<point_type, _N>& _verts
    ) noexcept
        : m_vertices(_verts)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   ray-casting point-in-polygon test in Cartesian space.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t crossings;

        auto cart_p = _System::to_cartesian(_p);
        crossings   = 0;

        // walk edges, count horizontal-ray crossings
        for (std::size_t i = 0, j = (_N - 1); i < _N; j = i++)
        {
            auto vi = _System::to_cartesian(m_vertices[i]);
            auto vj = _System::to_cartesian(m_vertices[j]);

            // standard ray-cast test
            if ( ((vi[1] > cart_p[1]) != (vj[1] > cart_p[1])) &&
                 ( cart_p[0] < ( (vj[0] - vi[0]) *
                                 (cart_p[1] - vi[1]) /
                                 (vj[1] - vi[1]) +
                                 vi[0] ) ) )
            {
                ++crossings;
            }
        }

        return ((crossings % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_perimeter
    //   sum of edge lengths (in Cartesian).
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type total;

        total = static_cast<value_type>(0);

        // sum cartesian distances around the boundary
        for (std::size_t i = 0; i < _N; ++i)
        {
            std::size_t j = (i + 1) % _N;

            total += cartesian_distance<_System>(m_vertices[i],
                                                  m_vertices[j]);
        }

        return total;
    }

    // closed_form_area
    //   absolute value of the shoelace formula in Cartesian.
    value_type
    closed_form_area
    () const noexcept
    {
        value_type sum;
        value_type result;

        sum = static_cast<value_type>(0);

        // shoelace cross-sum
        for (std::size_t i = 0; i < _N; ++i)
        {
            std::size_t j = (i + 1) % _N;

            auto vi = _System::to_cartesian(m_vertices[i]);
            auto vj = _System::to_cartesian(m_vertices[j]);

            sum += vi[0] * vj[1] - vj[0] * vi[1];
        }

        result = sum / static_cast<value_type>(2);

        // report absolute value (winding orientation is user-defined)
        if (result < static_cast<value_type>(0))
        {
            result = -result;
        }

        return result;
    }

    // closed_form_centroid
    //   standard signed-area centroid formula in Cartesian, with the
    // result mapped back into the user's coordinate system.
    point_type
    closed_form_centroid
    () const noexcept
    {
        value_type                cx;
        value_type                cy;
        value_type                area_sum;
        std::array<value_type, 2> c_cart;

        cx       = static_cast<value_type>(0);
        cy       = static_cast<value_type>(0);
        area_sum = static_cast<value_type>(0);

        // accumulate weighted contributions of each cross-term
        for (std::size_t i = 0; i < _N; ++i)
        {
            std::size_t j = (i + 1) % _N;

            auto vi = _System::to_cartesian(m_vertices[i]);
            auto vj = _System::to_cartesian(m_vertices[j]);

            value_type cross = vi[0] * vj[1] - vj[0] * vi[1];

            cx       += (vi[0] + vj[0]) * cross;
            cy       += (vi[1] + vj[1]) * cross;
            area_sum += cross;
        }

        // guard against degenerate polygons
        if (area_sum == static_cast<value_type>(0))
        {
            return m_vertices[0];
        }

        // 6A = 3 * (sum of crosses), so divide by 3*area_sum
        cx /= (static_cast<value_type>(3) * area_sum);
        cy /= (static_cast<value_type>(3) * area_sum);

        c_cart[0] = cx;
        c_cart[1] = cy;

        return _System::from_cartesian(c_cart);
    }
};


// ============================================================================
// II.   PARAMETRIC REGION
// ============================================================================

// parametric_region
//   struct: a planar region bounded by a single closed parametric
// curve. The user supplies a parametric_edge whose first and last
// sample points agree (closure is assumed, not enforced). Used for
// arbitrary smooth shapes like cardioids, lemniscates, and Lissajous
// blobs.
//
// Area, perimeter, and centroid are computed numerically by
// measure_2d via Green's theorem - no closed_form_* members are
// provided here.
template<typename _Edge>
struct parametric_region
{
    static_assert(is_edge<_Edge>::value,
                  "parametric_region: _Edge must satisfy the edge "
                  "interface.");
    static_assert(_Edge::dimension == 2,
                  "parametric_region: 2D edges only.");

    // ---- type aliases -------------------------------------------------------

    using boundary_type     = _Edge;
    using coordinate_system = typename _Edge::coordinate_system;
    using value_type        = typename _Edge::value_type;
    using point_type        = typename _Edge::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    boundary_type m_boundary;

    // ---- construction -------------------------------------------------------

    constexpr parametric_region() noexcept
        : m_boundary{}
    {
    }

    constexpr explicit parametric_region(
        const boundary_type& _b
    ) noexcept
        : m_boundary(_b)
    {
    }

    // ---- access -------------------------------------------------------------

    const boundary_type&
    boundary
    () const noexcept
    {
        return m_boundary;
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   tests containment by sampling the boundary into 256 polygon
    // vertices and running ray-cast. _Samples controls accuracy at the
    // cost of work.
    template<std::size_t _Samples = 256>
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t crossings;
        value_type  t_min;
        value_type  t_max;
        value_type  step;

        auto cart_p = coordinate_system::to_cartesian(_p);
        crossings   = 0;
        t_min       = m_boundary.parameter_min();
        t_max       = m_boundary.parameter_max();
        step        = (t_max - t_min) /
                      static_cast<value_type>(_Samples);

        // walk consecutive boundary samples as polygon edges
        for (std::size_t i = 0; i < _Samples; ++i)
        {
            value_type t1 = t_min +
                static_cast<value_type>(i) * step;
            value_type t2 = t_min +
                static_cast<value_type>(i + 1) * step;

            auto a = coordinate_system::to_cartesian(
                m_boundary.at(t1));
            auto b = coordinate_system::to_cartesian(
                m_boundary.at(t2));

            // ray-cast crossing predicate
            if ( ((a[1] > cart_p[1]) != (b[1] > cart_p[1])) &&
                 ( cart_p[0] < ( (b[0] - a[0]) *
                                 (cart_p[1] - a[1]) /
                                 (b[1] - a[1]) +
                                 a[0] ) ) )
            {
                ++crossings;
            }
        }

        return ((crossings % 2) == 1);
    }
};


// ============================================================================
// III.  IMPLICIT REGION
// ============================================================================

// implicit_region
//   struct: a planar region defined by F(x, y) ≤ 0. Wraps an
// expression from the math framework that satisfies the standard
// evaluate() interface and is expected to depend on two variables
// (multivariable indices 0 and 1).
//
// No closed_form_* measurements are provided; measure_2d would need
// to numerically integrate over a sampling grid, which is out of
// scope for this first cut.
template<typename _Expr,
         typename _System = cartesian<2, double>>
struct implicit_region
{
    static_assert(_System::dimension == 2,
                  "implicit_region: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using expression_type   = _Expr;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension   = 2;
    static constexpr bool        is_shape    = true;
    static constexpr bool        is_2d       = true;
    static constexpr bool        is_3d       = false;
    static constexpr bool        is_implicit = true;

    // ---- structural interface -----------------------------------------------

    // contains
    //   true when F(x, y) ≤ 0 in Cartesian.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto cart  = _System::to_cartesian(_p);
        auto input = std::make_tuple(cart[0], cart[1]);

        return ( _Expr::evaluate(input) <=
                 static_cast<value_type>(0) );
    }

    // on_boundary
    //   true when |F(x, y)| < _epsilon.
    bool
    on_boundary
    (
        const point_type& _p,
        value_type        _epsilon = static_cast<value_type>(1e-9)
    ) const noexcept
    {
        value_type v;

        auto cart  = _System::to_cartesian(_p);
        auto input = std::make_tuple(cart[0], cart[1]);

        v = _Expr::evaluate(input);

        if (v < static_cast<value_type>(0))
        {
            v = -v;
        }

        return (v < _epsilon);
    }
};


// ============================================================================
// IV.   SHAPE FROM EDGES (heterogeneous boundary)
// ============================================================================

// shape_from_edges
//   struct: a planar shape whose boundary is an ordered tuple of
// heterogeneous edges - e.g. two line_segments, an arc, and a
// cubic_bezier joined end-to-end. The user is responsible for ensuring
// closure (last edge's end matches first edge's start) and consistent
// winding.
//
// The coordinate system is taken from the first edge; all edges must
// share the same coordinate system.
template<typename... _Edges>
struct shape_from_edges
{
    static_assert(sizeof...(_Edges) >= 1,
                  "shape_from_edges: at least one edge required.");

    // ---- type aliases -------------------------------------------------------

    using first_edge_type = std::tuple_element_t<
        0, std::tuple<_Edges...>>;
    using coordinate_system =
        typename first_edge_type::coordinate_system;
    using value_type        =
        typename coordinate_system::value_type;
    using point_type        =
        typename coordinate_system::point_type;
    using edges_type        = std::tuple<_Edges...>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension =
        coordinate_system::dimension;
    static constexpr std::size_t edge_count = sizeof...(_Edges);
    static constexpr bool        is_shape   = true;
    static constexpr bool        is_2d      = (dimension == 2);
    static constexpr bool        is_3d      = (dimension == 3);

    // ---- data ---------------------------------------------------------------

    edges_type m_edges;

    // ---- construction -------------------------------------------------------

    constexpr shape_from_edges() noexcept
        : m_edges{}
    {
    }

    constexpr explicit shape_from_edges(
        const _Edges&... _e
    ) noexcept
        : m_edges(_e...)
    {
    }

    // ---- access -------------------------------------------------------------

    const edges_type&
    edges
    () const noexcept
    {
        return m_edges;
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   sampled ray-cast against every edge in the boundary. This is
    // a defensive fallback that works for any combination of edge
    // types; specialised shapes provide closed-form containment.
    template<std::size_t _SamplesPerEdge = 32>
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t crossings;

        auto cart_p = coordinate_system::to_cartesian(_p);
        crossings   = 0;

        // visit each edge in the tuple and accumulate ray-crossings
        std::apply(
            [&](const auto&... _edges)
            {
                ( count_edge_crossings<_SamplesPerEdge>(
                      _edges, cart_p, crossings),
                  ... );
            },
            m_edges);

        return ((crossings % 2) == 1);
    }

private:
    // count_edge_crossings
    //   helper: sub-samples a single edge into straight segments and
    // counts horizontal-ray crossings against _cart_p.
    template<std::size_t _SPE,
             typename    _SingleEdge>
    static void
    count_edge_crossings
    (
        const _SingleEdge&                          _edge,
        const std::array<value_type, dimension>&    _cart_p,
        std::size_t&                                _crossings
    ) noexcept
    {
        value_type t_min;
        value_type t_max;
        value_type step;

        t_min = _edge.parameter_min();
        t_max = _edge.parameter_max();
        step  = (t_max - t_min) /
                static_cast<value_type>(_SPE);

        // walk this edge's sub-segments
        for (std::size_t k = 0; k < _SPE; ++k)
        {
            value_type t1 = t_min +
                static_cast<value_type>(k) * step;
            value_type t2 = t_min +
                static_cast<value_type>(k + 1) * step;

            auto a = coordinate_system::to_cartesian(_edge.at(t1));
            auto b = coordinate_system::to_cartesian(_edge.at(t2));

            // standard ray-cast crossing predicate
            if ( ((a[1] > _cart_p[1]) != (b[1] > _cart_p[1])) &&
                 ( _cart_p[0] < ( (b[0] - a[0]) *
                                   (_cart_p[1] - a[1]) /
                                   (b[1] - a[1]) +
                                   a[0] ) ) )
            {
                ++_crossings;
            }
        }

        return;
    }
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SHAPE_2D_
