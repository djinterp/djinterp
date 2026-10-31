/*******************************************************************************
* djinterp [cpp/math]                                                   edge.hpp
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
*   line_segment<s>           - straight segment between two points
*   arc<s>                    - circular arc in 2D
*   parametric_edge<Curve, System> - wraps math::parametric_curve
*   implicit_edge<Expr, System>    - zero set of F(x,y) on a parameter
*                                    range (planar isocurves)
*   polyline_edge<System, N>       - sequence of N points, straight segs
*   quadratic_bezier<s>       - degree-2 Bezier
*   cubic_bezier<s>           - degree-3 Bezier
*
* LEVELS:
*   Everything compiles from C++98 but implicit_edge, which evaluates its
* expression on a std::tuple and so is C++11's. Points are linalg::vectors
* and the arithmetic theirs, in the formulas' former order; an arc's point
* is the coordinate core's polar conversion (the C library's cosine and
* sine at run time, as before); a polyline's points are a re_std::array
* (from C++11 a std::array of points converts).
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/edge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STRAIGHT EDGES
    --------------
    1.  line_segment
    2.  polyline_edge
2.  CURVED EDGES
    ------------
    1.  arc
    2.  quadratic_bezier
    3.  cubic_bezier
3.  WRAPPED EDGES
    -------------
    1.  parametric_edge
    2.  implicit_edge
*/

#ifndef DJINTERP_MATH_GEOMETRY_EDGE_HPP
#define DJINTERP_MATH_GEOMETRY_EDGE_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../../../../c/modules/math/coord.h"                   // d_coord_polar_to_
#if (defined(__cplusplus))
    #include "../../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
                                                  // cartesian_auto
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // cartesian_lerp,
                                                  // cartesian_distance
// re_std
#include "../../../../../re_std/cpp/array/array.hpp"        // re_std::array
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                              // std::array
    #include <tuple>                              // std::make_tuple
#endif


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  STRAIGHT EDGES
//==============================================================================


// 1.1    line_segment
//------------------------------------------------------------------------------
// 1.1.1
// line_segment
//   struct: a straight segment between two points in any coordinate
// system. Linearity is taken in Cartesian space -- at(t) converts the
// endpoints to Cartesian, lerps, and converts back -- the straight line
// through the two points however they happen to be expressed.
template<typename System>
struct line_segment
{
    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = System::dimension;
    static D_CONSTEXPR_VAR bool        is_edge   = true;
    static D_CONSTEXPR_VAR bool        is_curved = false;
    static D_CONSTEXPR_VAR bool        is_closed = false;

    point_type m_start;
    point_type m_end;

    D_CONSTEXPR
    line_segment() D_NOEXCEPT
        : m_start(),
          m_end()
    {}

    D_CONSTEXPR
    line_segment(
        const point_type& _start_point,
        const point_type& _end_point
    ) D_NOEXCEPT
        : m_start(_start_point),
          m_end(_end_point)
    {}

    D_CONSTEXPR
    value_type parameter_min() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    D_CONSTEXPR
    point_type start() const D_NOEXCEPT
    {
        return m_start;
    }

    D_CONSTEXPR
    point_type end() const D_NOEXCEPT
    {
        return m_end;
    }

    // at
    //   the point a fraction _t of the way along, in Cartesian space.
    D_CONSTEXPR_CPP14
    point_type at(
        value_type _t
    ) const D_NOEXCEPT
    {
        return cartesian_lerp<System>(m_start, m_end, _t);
    }

    // closed_form_length
    //   the straight-line distance between the endpoints.
    D_CONSTEXPR_CPP14
    value_type closed_form_length() const D_NOEXCEPT
    {
        return cartesian_distance<System>(m_start, m_end);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t line_segment<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool line_segment<System>::is_edge;
template<typename System>
D_CONSTEXPR_VAR bool line_segment<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool line_segment<System>::is_closed;

// 1.2    polyline_edge
//------------------------------------------------------------------------------
// 1.2.1
// polyline_edge
//   struct: N points joined by straight segments, parameterized over [0, N
// - 1], segment i over [i, i + 1].
template<typename    System,
         std::size_t N>
struct polyline_edge
{
    D_STATIC_ASSERT((N >= 2), "polyline_edge: need at least 2 points.");

    typedef System                       coordinate_system;
    typedef typename System::value_type  value_type;
    typedef typename System::point_type  point_type;
    typedef re_std::array<point_type, N> point_array;

    static D_CONSTEXPR_VAR std::size_t dimension   = System::dimension;
    static D_CONSTEXPR_VAR std::size_t point_count = N;
    static D_CONSTEXPR_VAR bool        is_edge     = true;
    static D_CONSTEXPR_VAR bool        is_curved   = false;
    static D_CONSTEXPR_VAR bool        is_closed   = false;

    point_array m_points;

    D_CONSTEXPR
    polyline_edge() D_NOEXCEPT
        : m_points()
    {}

    D_CONSTEXPR explicit
    polyline_edge(const point_array& _pts) D_NOEXCEPT
        : m_points(_pts)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from a std::array of points (C++11), as the points were held
    D_CONSTEXPR_CPP14 explicit
    polyline_edge(const std::array<point_type, N>& _pts) D_NOEXCEPT
        : m_points()
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            m_points[i] = _pts[i];
        }
    }
#endif

    D_CONSTEXPR
    value_type parameter_min() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max() const D_NOEXCEPT
    {
        return static_cast<value_type>(N - 1);
    }

    // at
    //   the point at _t, clamped into [0, N - 1]: segment floor(_t), the
    // last for _t = N - 1, lerped in Cartesian space.
    D_CONSTEXPR_CPP14
    point_type at(
        value_type _t
    ) const D_NOEXCEPT
    {
        const value_type last = static_cast<value_type>(N - 1);
        const value_type t    = (_t < static_cast<value_type>(0))
                                    ? static_cast<value_type>(0)
                                    : ((_t > last) ? last : _t);
        std::size_t      seg  = static_cast<std::size_t>(t);

        // the end of the last segment belongs to the last segment
        if (seg >= (N - 1))
        {
            seg = N - 2;
        }

        return cartesian_lerp<System>(m_points[seg],
                                      m_points[seg + 1],
                                      t - static_cast<value_type>(seg));
    }

    D_CONSTEXPR
    point_type start() const D_NOEXCEPT
    {
        return m_points[0];
    }

    D_CONSTEXPR
    point_type end() const D_NOEXCEPT
    {
        return m_points[N - 1];
    }

    // closed_form_length
    //   the segments' lengths, summed in order.
    D_CONSTEXPR_CPP14
    value_type closed_form_length() const D_NOEXCEPT
    {
        value_type total = static_cast<value_type>(0);

        for (std::size_t i = 0; (i + 1) < N; ++i)
        {
            total += cartesian_distance<System>(m_points[i], m_points[i + 1]);
        }

        return total;
    }
};

template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t polyline_edge<System, N>::dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polyline_edge<System, N>::is_edge;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polyline_edge<System, N>::is_curved;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polyline_edge<System, N>::is_closed;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t polyline_edge<System, N>::point_count;


//==============================================================================
// 2.  CURVED EDGES
//==============================================================================


// 2.1    arc
//------------------------------------------------------------------------------
// 2.1.1
// arc
//   struct: a circular arc in 2D, its angle swept linearly from
// m_start_angle (t = 0) to m_end_angle (t = 1).
template<typename System>
struct arc
{
    D_STATIC_ASSERT((System::dimension == 2),
                    "arc: only valid for 2D coordinate systems.");

    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = 2;
    static D_CONSTEXPR_VAR bool        is_edge   = true;
    static D_CONSTEXPR_VAR bool        is_curved = true;
    static D_CONSTEXPR_VAR bool        is_closed = false;

    point_type m_center;
    value_type m_radius;
    value_type m_start_angle;
    value_type m_end_angle;

    D_CONSTEXPR
    arc() D_NOEXCEPT
        : m_center(),
          m_radius(static_cast<value_type>(1)),
          m_start_angle(static_cast<value_type>(0)),
          m_end_angle(two_pi_of<value_type>())
    {}

    D_CONSTEXPR
    arc(
        const point_type& _center_point,
        value_type        _r,
        value_type        _start_a,
        value_type        _end_a
    ) D_NOEXCEPT
        : m_center(_center_point),
          m_radius(_r),
          m_start_angle(_start_a),
          m_end_angle(_end_a)
    {}

    D_CONSTEXPR
    value_type parameter_min() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // at
    //   centre + radius (cos a, sin a), a = start + t (end - start), in
    // Cartesian space.
    D_CONSTEXPR_CPP20
    point_type at(
        value_type _t
    ) const D_NOEXCEPT
    {
        const value_type angle = m_start_angle +
                                 (_t * (m_end_angle - m_start_angle));
        const point_type polar(m_radius, angle);
        point_type       offset;

        d_coord_polar_to_cartesian_auto(offset.begin(), polar.data());

        return System::from_cartesian(
            System::to_cartesian(m_center).plus(offset));
    }

    D_CONSTEXPR_CPP20
    point_type start() const D_NOEXCEPT
    {
        return at(static_cast<value_type>(0));
    }

    D_CONSTEXPR_CPP20
    point_type end() const D_NOEXCEPT
    {
        return at(static_cast<value_type>(1));
    }

    // closed_form_length
    //   radius times the sweep.
    D_CONSTEXPR_CPP14
    value_type closed_form_length() const D_NOEXCEPT
    {
        const value_type sweep = m_end_angle - m_start_angle;

        return m_radius * ((sweep < static_cast<value_type>(0)) ? -sweep
                                                                : sweep);
    }

    // is_full_circle
    //   whether the sweep is a whole turn, within _epsilon.
    D_CONSTEXPR_CPP14
    bool is_full_circle(
        value_type _epsilon = static_cast<value_type>(1e-9)
    ) const D_NOEXCEPT
    {
        const value_type raw   = m_end_angle - m_start_angle;
        const value_type sweep = (raw < static_cast<value_type>(0)) ? -raw
                                                                    : raw;
        const value_type diff  = sweep - two_pi_of<value_type>();

        return (((diff < static_cast<value_type>(0)) ? -diff : diff) <
                _epsilon);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t arc<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool arc<System>::is_edge;
template<typename System>
D_CONSTEXPR_VAR bool arc<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool arc<System>::is_closed;

// 2.2    quadratic_bezier
//------------------------------------------------------------------------------
// 2.2.1
// quadratic_bezier
//   struct: the degree-2 Bezier curve, (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2,
// in Cartesian space.
template<typename System>
struct quadratic_bezier
{
    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = System::dimension;
    static D_CONSTEXPR_VAR bool        is_edge   = true;
    static D_CONSTEXPR_VAR bool        is_curved = true;
    static D_CONSTEXPR_VAR bool        is_closed = false;

    point_type m_p0;
    point_type m_p1;
    point_type m_p2;

    D_CONSTEXPR
    quadratic_bezier() D_NOEXCEPT
        : m_p0(),
          m_p1(),
          m_p2()
    {}

    D_CONSTEXPR
    quadratic_bezier(
        const point_type& _p0,
        const point_type& _p1,
        const point_type& _p2
    ) D_NOEXCEPT
        : m_p0(_p0),
          m_p1(_p1),
          m_p2(_p2)
    {}

    D_CONSTEXPR
    value_type parameter_min() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // at
    //   the Bernstein weights' combination, summed in order.
    D_CONSTEXPR_CPP14
    point_type at(
        value_type _t
    ) const D_NOEXCEPT
    {
        const value_type u  = static_cast<value_type>(1) - _t;
        const value_type w0 = u * u;
        const value_type w1 = static_cast<value_type>(2) * u * _t;
        const value_type w2 = _t * _t;

        return System::from_cartesian(
            System::to_cartesian(m_p0).scaled(w0)
                .plus(System::to_cartesian(m_p1).scaled(w1))
                .plus(System::to_cartesian(m_p2).scaled(w2)));
    }

    D_CONSTEXPR
    point_type start() const D_NOEXCEPT
    {
        return m_p0;
    }

    D_CONSTEXPR
    point_type end() const D_NOEXCEPT
    {
        return m_p2;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t quadratic_bezier<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool quadratic_bezier<System>::is_edge;
template<typename System>
D_CONSTEXPR_VAR bool quadratic_bezier<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool quadratic_bezier<System>::is_closed;

// 2.3    cubic_bezier
//------------------------------------------------------------------------------
// 2.3.1
// cubic_bezier
//   struct: the degree-3 Bezier curve, (1-t)^3 p0 + 3(1-t)^2 t p1 + 3(1-t)
// t^2 p2 + t^3 p3, in Cartesian space.
template<typename System>
struct cubic_bezier
{
    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = System::dimension;
    static D_CONSTEXPR_VAR bool        is_edge   = true;
    static D_CONSTEXPR_VAR bool        is_curved = true;
    static D_CONSTEXPR_VAR bool        is_closed = false;

    point_type m_p0;
    point_type m_p1;
    point_type m_p2;
    point_type m_p3;

    D_CONSTEXPR
    cubic_bezier() D_NOEXCEPT
        : m_p0(),
          m_p1(),
          m_p2(),
          m_p3()
    {}

    D_CONSTEXPR
    cubic_bezier(
        const point_type& _p0,
        const point_type& _p1,
        const point_type& _p2,
        const point_type& _p3
    ) D_NOEXCEPT
        : m_p0(_p0),
          m_p1(_p1),
          m_p2(_p2),
          m_p3(_p3)
    {}

    D_CONSTEXPR
    value_type parameter_min() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // at
    //   the Bernstein weights' combination, summed in order.
    D_CONSTEXPR_CPP14
    point_type at(
        value_type _t
    ) const D_NOEXCEPT
    {
        const value_type u  = static_cast<value_type>(1) - _t;
        const value_type uu = u * u;
        const value_type tt = _t * _t;
        const value_type w0 = uu * u;
        const value_type w1 = static_cast<value_type>(3) * uu * _t;
        const value_type w2 = static_cast<value_type>(3) * u * tt;
        const value_type w3 = tt * _t;

        return System::from_cartesian(
            System::to_cartesian(m_p0).scaled(w0)
                .plus(System::to_cartesian(m_p1).scaled(w1))
                .plus(System::to_cartesian(m_p2).scaled(w2))
                .plus(System::to_cartesian(m_p3).scaled(w3)));
    }

    D_CONSTEXPR
    point_type start() const D_NOEXCEPT
    {
        return m_p0;
    }

    D_CONSTEXPR
    point_type end() const D_NOEXCEPT
    {
        return m_p3;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t cubic_bezier<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool cubic_bezier<System>::is_edge;
template<typename System>
D_CONSTEXPR_VAR bool cubic_bezier<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool cubic_bezier<System>::is_closed;


//==============================================================================
// 3.  WRAPPED EDGES
//==============================================================================


// 3.1    parametric_edge
//------------------------------------------------------------------------------
// 3.1.1
// parametric_edge
//   struct: wraps a parametric curve type -- a static evaluate(t) returning
// an indexable point, and a static output_dimension -- over [t_min, t_max].
template<typename ParametricCurve,
         typename System = cartesian<ParametricCurve::output_dimension,
                                     double> >
struct parametric_edge
{
    typedef System                      coordinate_system;
    typedef ParametricCurve             parametric_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = System::dimension;
    static D_CONSTEXPR_VAR bool        is_edge   = true;
    static D_CONSTEXPR_VAR bool        is_curved = true;
    static D_CONSTEXPR_VAR bool        is_closed = false;

    D_STATIC_ASSERT((ParametricCurve::output_dimension == System::dimension),
                    "parametric_edge: curve output dimension must match "
                    "coordinate system dimension.");

    value_type m_t_min;
    value_type m_t_max;

    D_CONSTEXPR
    parametric_edge() D_NOEXCEPT
        : m_t_min(static_cast<value_type>(0)),
          m_t_max(static_cast<value_type>(1))
    {}

    D_CONSTEXPR
    parametric_edge(
        value_type _t_lower,
        value_type _t_upper
    ) D_NOEXCEPT
        : m_t_min(_t_lower),
          m_t_max(_t_upper)
    {}

    D_CONSTEXPR value_type
    parameter_min() const D_NOEXCEPT
    {
        return m_t_min;
    }

    D_CONSTEXPR value_type
    parameter_max() const D_NOEXCEPT
    {
        return m_t_max;
    }

    // at
    //   the wrapped curve's point at _t, in this system (a Cartesian
    // system's from_cartesian is the point itself).
    point_type
    at(value_type _t) const
    {
        return from_raw(ParametricCurve::evaluate(_t));
    }

    point_type
    start() const
    {
        return at(m_t_min);
    }

    point_type
    end() const
    {
        return at(m_t_max);
    }

private:
    // from_raw
    //   the evaluated point, its components converted -- a template, so the
    // result's type is deduced at every level.
    template<typename Raw>
    static point_type
    from_raw(const Raw& _raw)
    {
        point_type cart;

        for (std::size_t i = 0; i < dimension; ++i)
        {
            cart[i] = static_cast<value_type>(_raw[i]);
        }

        return System::from_cartesian(cart);
    }
};

template<typename ParametricCurve, typename System>
D_CONSTEXPR_VAR std::size_t parametric_edge<ParametricCurve, System>::dimension;
template<typename ParametricCurve, typename System>
D_CONSTEXPR_VAR bool parametric_edge<ParametricCurve, System>::is_edge;
template<typename ParametricCurve, typename System>
D_CONSTEXPR_VAR bool parametric_edge<ParametricCurve, System>::is_curved;
template<typename ParametricCurve, typename System>
D_CONSTEXPR_VAR bool parametric_edge<ParametricCurve, System>::is_closed;

// 3.2    implicit_edge
//------------------------------------------------------------------------------
// From C++11: its expression evaluates on a std::tuple.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// 3.2.1
// implicit_edge
//   struct: the zero set of F(x, y) in the plane, F a type with a static
// evaluate(std::tuple of x, y).
template<typename Expr,
         typename System = cartesian<2, double> >
struct implicit_edge
{
    D_STATIC_ASSERT((System::dimension == 2),
                    "implicit_edge: planar 2D systems only.");

    typedef System                      coordinate_system;
    typedef Expr                        expression_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static constexpr std::size_t dimension   = 2;
    static constexpr bool        is_edge     = true;
    static constexpr bool        is_curved   = true;
    static constexpr bool        is_closed   = false;
    static constexpr bool        is_implicit = true;

    // on_curve
    //   |F(p)| < _epsilon, on p's Cartesian form.
    bool
    on_curve(
        const point_type& _p,
        value_type        _epsilon = static_cast<value_type>(1e-9)
    ) const
    {
        const point_type cart = System::to_cartesian(_p);
        value_type       v    = Expr::evaluate(std::make_tuple(cart[0],
                                                               cart[1]));

        if (v < static_cast<value_type>(0))
        {
            v = -v;
        }

        return (v < _epsilon);
    }
};

template<typename Expr, typename System>
constexpr std::size_t implicit_edge<Expr, System>::dimension;
template<typename Expr, typename System>
constexpr bool implicit_edge<Expr, System>::is_edge;
template<typename Expr, typename System>
constexpr bool implicit_edge<Expr, System>::is_curved;
template<typename Expr, typename System>
constexpr bool implicit_edge<Expr, System>::is_closed;
template<typename Expr, typename System>
constexpr bool implicit_edge<Expr, System>::is_implicit;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_EDGE_HPP
