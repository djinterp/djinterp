/******************************************************************************
* djinterp [math]                                                      edge.hpp
*
* Dimension-agnostic edge / curve primitives.
*   Provides edge types satisfying the structural edge interface defined
* in geometry_common.hpp. Most edges live in any coordinate system of
* any dimension; arcs and Bezier curves use Cartesian-space arithmetic
* internally and pivot through to_cartesian / from_cartesian to keep
* the geometric meaning correct in curvilinear systems. Arcs are 2D
* only by static_assert; everything else is dimension-portable.
*
* PROVIDED TYPES:
*   line_segment<System>           - straight segment between two points
*   arc<System>                    - circular arc in 2D
*   parametric_edge<Curve, System> - wraps math::parametric_curve
*   implicit_edge<Expr, System>    - zero set of F(x,y) on a parameter
*                                    range (planar isocurves)
*   polyline_edge<System, N>       - sequence of N points, straight segs
*   quadratic_bezier<System>       - degree-2 Bezier
*   cubic_bezier<System>           - degree-3 Bezier
*
* path:      /inc/djinterp/math/geometry/edge.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_EDGE_
#define DJINTERP_MATH_GEOMETRY_EDGE_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <tuple>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    LINE SEGMENT
// ============================================================================

// line_segment
//   struct: a straight segment between two points in any coordinate
// system. Linearity is taken in Cartesian space - the at(t) call
// converts the endpoints to Cartesian, lerps, and converts back. This
// matches the standard geometric reading of "the straight line through
// these two points," independent of how the endpoints happened to be
// expressed.
template<typename _System>
struct line_segment
{
    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = _System::dimension;
    static constexpr bool        is_edge   = true;
    static constexpr bool        is_curved = false;
    static constexpr bool        is_closed = false;

    // ---- data ---------------------------------------------------------------

    point_type m_start;
    point_type m_end;

    // ---- construction -------------------------------------------------------

    constexpr line_segment() noexcept
        : m_start{},
          m_end{}
    {
    }

    constexpr line_segment(
        const point_type& _start_point,
        const point_type& _end_point
    ) noexcept
        : m_start(_start_point),
          m_end(_end_point)
    {
    }

    // ---- structural interface -----------------------------------------------

    // parameter_min / parameter_max
    //   parameter convention for a segment is t ∈ [0, 1].
    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // start / end
    constexpr point_type
    start
    () const noexcept
    {
        return m_start;
    }

    constexpr point_type
    end
    () const noexcept
    {
        return m_end;
    }

    // at
    //   linear interpolation in Cartesian space.
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        return cartesian_lerp<_System>(m_start, m_end, _t);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_length
    //   straight-line Euclidean distance between endpoints.
    value_type
    closed_form_length
    () const noexcept
    {
        return cartesian_distance<_System>(m_start, m_end);
    }
};


// ============================================================================
// II.   CIRCULAR ARC (2D ONLY)
// ============================================================================

// arc
//   struct: a circular arc in 2D, parameterised by center, radius,
// start angle, and end angle (all in radians, ccw positive). Sampling
// is done in Cartesian space and converted back into the chosen
// coordinate system, so an arc anchored in polar coordinates still
// traces the geometric circle the user expects.
template<typename _System>
struct arc
{
    static_assert(_System::dimension == 2,
                  "arc: only valid for 2D coordinate systems.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_edge   = true;
    static constexpr bool        is_curved = true;
    static constexpr bool        is_closed = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;
    value_type m_start_angle;
    value_type m_end_angle;

    // ---- construction -------------------------------------------------------

    constexpr arc() noexcept
        : m_center{},
          m_radius(static_cast<value_type>(1)),
          m_start_angle(static_cast<value_type>(0)),
          m_end_angle(two_pi_v<value_type>)
    {
    }

    constexpr arc(
        const point_type& _center_point,
        value_type        _r,
        value_type        _start_a,
        value_type        _end_a
    ) noexcept
        : m_center(_center_point),
          m_radius(_r),
          m_start_angle(_start_a),
          m_end_angle(_end_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // at
    //   samples the arc at t ∈ [0, 1].
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        value_type                  angle;
        std::array<value_type, 2>   cc;
        std::array<value_type, 2>   cart;

        // compute angle and convert center to cartesian
        angle = m_start_angle +
                _t * (m_end_angle - m_start_angle);
        cc    = _System::to_cartesian(m_center);

        cart[0] = cc[0] + m_radius * std::cos(angle);
        cart[1] = cc[1] + m_radius * std::sin(angle);

        return _System::from_cartesian(cart);
    }

    point_type
    start
    () const noexcept
    {
        return at(static_cast<value_type>(0));
    }

    point_type
    end
    () const noexcept
    {
        return at(static_cast<value_type>(1));
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_length
    //   arc length = r * |sweep|.
    value_type
    closed_form_length
    () const noexcept
    {
        value_type sweep;

        sweep = m_end_angle - m_start_angle;

        // take absolute value
        if (sweep < static_cast<value_type>(0))
        {
            sweep = -sweep;
        }

        return m_radius * sweep;
    }

    // ---- queries ------------------------------------------------------------

    // is_full_circle
    //   true if |sweep| is approximately 2π.
    bool
    is_full_circle
    (
        value_type _epsilon = static_cast<value_type>(1e-9)
    ) const noexcept
    {
        value_type sweep;
        value_type diff;

        sweep = m_end_angle - m_start_angle;

        if (sweep < static_cast<value_type>(0))
        {
            sweep = -sweep;
        }

        diff = sweep - two_pi_v<value_type>;

        if (diff < static_cast<value_type>(0))
        {
            diff = -diff;
        }

        return (diff < _epsilon);
    }
};


// ============================================================================
// III.  PARAMETRIC EDGE (wraps math::parametric_curve)
// ============================================================================

// parametric_edge
//   struct: a bounded portion of a parametric curve from the math
// framework. The wrapped curve's evaluate(t) returns Cartesian
// components; this edge stores a (t_min, t_max) interval as runtime
// members and converts each sampled point into the user-chosen
// coordinate system.
//
// Usage:
//   using my_curve = parametric_curve<cos_fn<var_t<>>, sin_fn<var_t<>>>;
//   parametric_edge<my_curve> e{0.0, two_pi_v<double>};  // unit circle
template<typename _ParametricCurve,
         typename _System = cartesian<_ParametricCurve::output_dimension,
                                       double>>
struct parametric_edge
{
    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using parametric_type   = _ParametricCurve;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = _System::dimension;
    static constexpr bool        is_edge   = true;
    static constexpr bool        is_curved = true;
    static constexpr bool        is_closed = false;

    static_assert(_ParametricCurve::output_dimension == _System::dimension,
                  "parametric_edge: curve output dimension must match "
                  "coordinate system dimension.");

    // ---- data ---------------------------------------------------------------

    value_type m_t_min;
    value_type m_t_max;

    // ---- construction -------------------------------------------------------

    constexpr parametric_edge() noexcept
        : m_t_min(static_cast<value_type>(0)),
          m_t_max(static_cast<value_type>(1))
    {
    }

    constexpr parametric_edge(
        value_type _t_lower,
        value_type _t_upper
    ) noexcept
        : m_t_min(_t_lower),
          m_t_max(_t_upper)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min
    () const noexcept
    {
        return m_t_min;
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return m_t_max;
    }

    // at
    //   evaluates the wrapped parametric_curve at _t and (when the
    // chosen coordinate system isn't already Cartesian) maps the result
    // into the system's native point type.
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        std::array<value_type, dimension> cart{};

        // wrapped curve returns std::array<double, dim> in cartesian
        auto raw = _ParametricCurve::evaluate(_t);

        for (std::size_t i = 0; i < dimension; ++i)
        {
            cart[i] = static_cast<value_type>(raw[i]);
        }

        // identity if the user's system is already cartesian
        if constexpr (_System::is_cartesian)
        {
            return cart;
        }
        else
        {
            return _System::from_cartesian(cart);
        }
    }

    point_type
    start
    () const noexcept
    {
        return at(m_t_min);
    }

    point_type
    end
    () const noexcept
    {
        return at(m_t_max);
    }
};


// ============================================================================
// IV.   IMPLICIT EDGE
// ============================================================================

// implicit_edge
//   struct: a curve defined implicitly by F(x, y) = 0, restricted to
// some user-supplied parameter range. Unlike parametric_edge, this
// type does not provide at(t) - the curve isn't parameterised. It is
// useful as a containment predicate and as a boundary type when the
// user wants to express a region by an implicit equation.
//
// For sampled traversal of an implicit curve, the user should convert
// to a parametric form (e.g. via marching-squares or a level-set
// tracer) before constructing an edge.
template<typename _Expr,
         typename _System = cartesian<2, double>>
struct implicit_edge
{
    static_assert(_System::dimension == 2,
                  "implicit_edge: planar 2D systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using expression_type   = _Expr;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension     = 2;
    static constexpr bool        is_edge       = true;
    static constexpr bool        is_curved     = true;
    static constexpr bool        is_closed     = false;
    static constexpr bool        is_implicit   = true;

    // ---- structural interface (degenerate) ----------------------------------

    // parameter_min / parameter_max
    //   implicit edges are not parameterised; we expose [0, 1] so the
    // structural interface still type-checks, but at(t) is not defined.
    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // ---- containment --------------------------------------------------------

    // on_curve
    //   true when |F(x, y)| < _epsilon, i.e. the point lies on the
    // implicit curve up to numerical tolerance.
    bool
    on_curve
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
// V.    POLYLINE EDGE
// ============================================================================

// polyline_edge
//   struct: a sequence of _N points connected by straight segments.
// Parameter t runs from 0 to N-1; the integer part picks the segment
// and the fractional part lerps within it.
template<typename    _System,
         std::size_t _N>
struct polyline_edge
{
    static_assert(_N >= 2,
                  "polyline_edge: need at least 2 points.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension   = _System::dimension;
    static constexpr std::size_t point_count = _N;
    static constexpr bool        is_edge     = true;
    static constexpr bool        is_curved   = false;
    static constexpr bool        is_closed   = false;

    // ---- data ---------------------------------------------------------------

    std::array<point_type, _N> m_points;

    // ---- construction -------------------------------------------------------

    constexpr polyline_edge() noexcept
        : m_points{}
    {
    }

    constexpr explicit polyline_edge(
        const std::array<point_type, _N>& _pts
    ) noexcept
        : m_points(_pts)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(_N - 1);
    }

    // at
    //   floor(_t) picks the segment index; _t - floor(_t) is the local
    // parameter within that segment.
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        std::size_t seg;
        value_type  local;

        // clamp parameter into [0, _N - 1]
        if (_t < static_cast<value_type>(0))
        {
            _t = static_cast<value_type>(0);
        }

        if (_t > static_cast<value_type>(_N - 1))
        {
            _t = static_cast<value_type>(_N - 1);
        }

        seg = static_cast<std::size_t>(_t);

        // clamp segment index for the right endpoint
        if (seg >= (_N - 1))
        {
            seg = _N - 2;
        }

        local = _t - static_cast<value_type>(seg);

        return cartesian_lerp<_System>(m_points[seg],
                                        m_points[seg + 1],
                                        local);
    }

    point_type
    start
    () const noexcept
    {
        return m_points[0];
    }

    point_type
    end
    () const noexcept
    {
        return m_points[_N - 1];
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_length
    //   sum of segment lengths.
    value_type
    closed_form_length
    () const noexcept
    {
        value_type total;

        total = static_cast<value_type>(0);

        // walk every consecutive pair of vertices
        for (std::size_t i = 0; (i + 1) < _N; ++i)
        {
            total += cartesian_distance<_System>(m_points[i],
                                                  m_points[i + 1]);
        }

        return total;
    }
};


// ============================================================================
// VI.   QUADRATIC BEZIER
// ============================================================================

// quadratic_bezier
//   struct: B(t) = (1-t)² P0 + 2(1-t)t P1 + t² P2, with t ∈ [0, 1].
// Control points are stored in the user's coordinate system; the
// Bernstein blend is computed in Cartesian space.
template<typename _System>
struct quadratic_bezier
{
    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = _System::dimension;
    static constexpr bool        is_edge   = true;
    static constexpr bool        is_curved = true;
    static constexpr bool        is_closed = false;

    // ---- data ---------------------------------------------------------------

    point_type m_p0;
    point_type m_p1;
    point_type m_p2;

    // ---- construction -------------------------------------------------------

    constexpr quadratic_bezier() noexcept
        : m_p0{},
          m_p1{},
          m_p2{}
    {
    }

    constexpr quadratic_bezier(
        const point_type& _p0,
        const point_type& _p1,
        const point_type& _p2
    ) noexcept
        : m_p0(_p0),
          m_p1(_p1),
          m_p2(_p2)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // at
    //   Bernstein blend in Cartesian.
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        value_type                          u;
        value_type                          w0;
        value_type                          w1;
        value_type                          w2;
        std::array<value_type, dimension>   p;

        u  = static_cast<value_type>(1) - _t;
        w0 = u * u;
        w1 = static_cast<value_type>(2) * u * _t;
        w2 = _t * _t;

        auto c0 = _System::to_cartesian(m_p0);
        auto c1 = _System::to_cartesian(m_p1);
        auto c2 = _System::to_cartesian(m_p2);

        for (std::size_t i = 0; i < dimension; ++i)
        {
            p[i] = w0 * c0[i] + w1 * c1[i] + w2 * c2[i];
        }

        return _System::from_cartesian(p);
    }

    point_type
    start
    () const noexcept
    {
        return m_p0;
    }

    point_type
    end
    () const noexcept
    {
        return m_p2;
    }
};


// ============================================================================
// VII.  CUBIC BEZIER
// ============================================================================

// cubic_bezier
//   struct: B(t) = (1-t)³ P0 + 3(1-t)²t P1 + 3(1-t)t² P2 + t³ P3.
template<typename _System>
struct cubic_bezier
{
    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = _System::dimension;
    static constexpr bool        is_edge   = true;
    static constexpr bool        is_curved = true;
    static constexpr bool        is_closed = false;

    // ---- data ---------------------------------------------------------------

    point_type m_p0;
    point_type m_p1;
    point_type m_p2;
    point_type m_p3;

    // ---- construction -------------------------------------------------------

    constexpr cubic_bezier() noexcept
        : m_p0{},
          m_p1{},
          m_p2{},
          m_p3{}
    {
    }

    constexpr cubic_bezier(
        const point_type& _p0,
        const point_type& _p1,
        const point_type& _p2,
        const point_type& _p3
    ) noexcept
        : m_p0(_p0),
          m_p1(_p1),
          m_p2(_p2),
          m_p3(_p3)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // at
    //   cubic Bernstein blend in Cartesian space.
    point_type
    at
    (
        value_type _t
    ) const noexcept
    {
        value_type                          u;
        value_type                          uu;
        value_type                          tt;
        value_type                          w0;
        value_type                          w1;
        value_type                          w2;
        value_type                          w3;
        std::array<value_type, dimension>   p;

        u  = static_cast<value_type>(1) - _t;
        uu = u * u;
        tt = _t * _t;
        w0 = uu * u;
        w1 = static_cast<value_type>(3) * uu * _t;
        w2 = static_cast<value_type>(3) * u  * tt;
        w3 = tt * _t;

        auto c0 = _System::to_cartesian(m_p0);
        auto c1 = _System::to_cartesian(m_p1);
        auto c2 = _System::to_cartesian(m_p2);
        auto c3 = _System::to_cartesian(m_p3);

        for (std::size_t i = 0; i < dimension; ++i)
        {
            p[i] = w0 * c0[i] +
                   w1 * c1[i] +
                   w2 * c2[i] +
                   w3 * c3[i];
        }

        return _System::from_cartesian(p);
    }

    point_type
    start
    () const noexcept
    {
        return m_p0;
    }

    point_type
    end
    () const noexcept
    {
        return m_p3;
    }
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_EDGE_
