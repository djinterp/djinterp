/******************************************************************************
* djinterp [math]                                                   star_2d.hpp
*
* 2D star shapes.
*   Provides two complementary constructions of "star-shaped" planar
* figures, plus a small catalogue of named instances. The two
* constructions answer different questions:
*
*   - star_polygon<System, N, K>:
*       the classic Schlafli {n/k} star polygon, drawn by connecting
*       every k-th vertex of a regular n-gon inscribed in a circle.
*       Pentagram = {5/2}, octagram = {8/3}, etc. Self-intersecting.
*
*   - n_pointed_star<System, N>:
*       the everyday "5-point star" with outer points and inner
*       valleys, modelled as a simple 2N-vertex non-convex polygon
*       alternating between two circles of different radii.
*
* Both satisfy the structural shape interface from
* geometry_common.hpp. star_polygon uses even-odd containment (so the
* inner sub-polygon is hollow, matching the {n/k} ring rendering);
* n_pointed_star is simple, so all rules agree.
*
* CLOSED-FORM MEASUREMENTS:
*   - n_pointed_star has clean closed-form area, perimeter, centroid.
*   - star_polygon has closed-form perimeter (n equal chords) and a
*     closed-form *signed* area (n/2)·R²·sin(2πk/n), exposed as
*     closed_form_signed_area(). Even-odd area for {n/k} is the signed
*     area minus twice the inner polygon's area; computing it
*     analytically requires the inner-polygon radius which depends on
*     the chord intersections, so we leave that to measure_2d's
*     numerical path for users who really need it.
*
* PROVIDED TYPES:
*   star_polygon<System, N, K>     - {n/k} Schlafli star polygon
*   n_pointed_star<System, N>      - simple star with N outer points
*   pentagram<System>              - alias for star_polygon<.., 5, 2>
*   hexagram<System>               - alias for star_polygon<.., 6, 2>
*                                    (a compound of two triangles)
*   hexagonal_star<System>         - the Star of David as a simple
*                                    12-vertex non-convex polygon
*
* path:      /inc/djinterp/math/geometry/star_2d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_STAR_2D_
#define DJINTERP_MATH_GEOMETRY_STAR_2D_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    STAR POLYGON {n/k}
// ============================================================================

// star_polygon
//   struct: the Schlafli star polygon {N/K} - inscribe N evenly-
// spaced vertices in a circle of radius _circumradius and connect
// every K-th vertex with straight edges. Valid for K satisfying
// 2 <= K < N and gcd(N, K) == 1 (so the path visits every vertex
// before closing).
//
// When gcd(N, K) != 1, the figure is a star *compound* of gcd(N,K)
// smaller star polygons; we still allow construction in that case
// because it's geometrically meaningful (hexagram = {6/2} is two
// overlapping triangles).
//
// Containment uses the even-odd rule on the self-intersecting
// polyline boundary, so {5/2} renders as a pentagram with a hollow
// pentagonal centre.
template<typename    _System = cartesian<2, double>,
         std::size_t _N      = 5,
         std::size_t _K      = 2>
struct star_polygon
{
    static_assert(_System::dimension == 2,
                  "star_polygon: 2D coordinate systems only.");
    static_assert(_N >= 5,
                  "star_polygon: need at least 5 vertices for a "
                  "non-degenerate {n/k}.");
    static_assert(_K >= 2,
                  "star_polygon: K must be >= 2 (K = 1 gives a "
                  "regular polygon - use regular_polygon).");
    static_assert((_K * 2) < _N,
                  "star_polygon: K must be < N/2 (K = N/2 is "
                  "degenerate, K > N/2 is reflection of K < N/2).");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension  = 2;
    static constexpr std::size_t vertex_n   = _N;
    static constexpr std::size_t step_k     = _K;
    static constexpr bool        is_shape   = true;
    static constexpr bool        is_2d      = true;
    static constexpr bool        is_3d      = false;
    static constexpr bool        is_star    = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_circumradius;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    constexpr star_polygon() noexcept
        : m_center{},
          m_circumradius(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {
    }

    constexpr star_polygon(
        const point_type& _centre,
        value_type        _r,
        value_type        _rot = static_cast<value_type>(0)
    ) noexcept
        : m_center(_centre),
          m_circumradius(_r),
          m_rotation(_rot)
    {
    }

    // ---- vertex enumeration -------------------------------------------------

    // vertex
    //   returns the _i-th vertex (0 <= _i < _N) in the user's
    // coordinate system. Vertices are laid out around the circle in
    // the natural angular order; the K-step pattern is applied when
    // walking the boundary, not when indexing vertices.
    point_type
    vertex
    (
        std::size_t _i
    ) const noexcept
    {
        value_type                angle;
        std::array<value_type, 2> v_cart;

        angle = m_rotation +
                two_pi_v<value_type> *
                static_cast<value_type>(_i) /
                static_cast<value_type>(_N);

        auto c_cart = _System::to_cartesian(m_center);

        v_cart[0] = c_cart[0] + m_circumradius * std::cos(angle);
        v_cart[1] = c_cart[1] + m_circumradius * std::sin(angle);

        return _System::from_cartesian(v_cart);
    }

    // boundary_vertex
    //   returns the _i-th vertex along the BOUNDARY traversal order.
    // The boundary visits vertices in K-step order: 0, K, 2K, 3K, ...
    // mod N. Returns to the start after N steps (assuming
    // gcd(N, K) == 1).
    point_type
    boundary_vertex
    (
        std::size_t _i
    ) const noexcept
    {
        return vertex((_i * _K) % _N);
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   even-odd ray-cast against the K-step boundary path. This
    // produces the conventional pentagram-with-hollow-pentagon
    // rendering for {5/2}.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t crossings;

        auto p_cart = _System::to_cartesian(_p);
        crossings   = 0;

        // walk the K-step boundary
        for (std::size_t step = 0; step < _N; ++step)
        {
            std::size_t i_idx = (step * _K) % _N;
            std::size_t j_idx = ((step + 1) * _K) % _N;

            auto vi = _System::to_cartesian(vertex(i_idx));
            auto vj = _System::to_cartesian(vertex(j_idx));

            // standard ray-cast crossing predicate
            if ( ((vi[1] > p_cart[1]) != (vj[1] > p_cart[1])) &&
                 ( p_cart[0] < ( (vj[0] - vi[0]) *
                                  (p_cart[1] - vi[1]) /
                                  (vj[1] - vi[1]) +
                                  vi[0] ) ) )
            {
                ++crossings;
            }
        }

        // even-odd fill
        return ((crossings % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_perimeter
    //   N equal chords, each of length 2 R sin(K π / N).
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type theta;

        // angle subtended by one K-step chord
        theta = pi_v<value_type> *
                static_cast<value_type>(_K) /
                static_cast<value_type>(_N);

        return static_cast<value_type>(2) *
               static_cast<value_type>(_N) *
               m_circumradius * std::sin(theta);
    }

    // closed_form_signed_area
    //   the analytic signed area of the {N/K} star polygon:
    //   A_signed = (N/2) R² sin(2 π K / N)
    // This is the area "swept" by the boundary walking K steps each
    // time; for the conventional (even-odd) fill of {5/2} it counts
    // the centre pentagon twice, so even-odd area is smaller. See
    // header notes for the difference.
    value_type
    closed_form_signed_area
    () const noexcept
    {
        value_type theta;

        theta = two_pi_v<value_type> *
                static_cast<value_type>(_K) /
                static_cast<value_type>(_N);

        return static_cast<value_type>(0.5) *
               static_cast<value_type>(_N) *
               m_circumradius * m_circumradius *
               std::sin(theta);
    }

    // closed_form_centroid
    //   by symmetry, the centre.
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// II.   N-POINTED STAR (simple non-convex polygon)
// ============================================================================

// n_pointed_star
//   struct: the everyday "N-point star" shape, modelled as a simple
// (non-self-intersecting) polygon with 2N vertices that alternate
// between an outer circle of radius R_outer and an inner circle of
// radius R_inner. The outer vertices are the points of the star;
// the inner vertices are the valleys between them. Vertex 0 is an
// outer point at angle m_rotation.
//
// Unlike star_polygon, this is a Jordan curve, so even-odd and
// nonzero-winding both yield the same filled-star shape and the
// area formula is exact and elementary.
template<typename    _System = cartesian<2, double>,
         std::size_t _N      = 5>
struct n_pointed_star
{
    static_assert(_System::dimension == 2,
                  "n_pointed_star: 2D coordinate systems only.");
    static_assert(_N >= 3,
                  "n_pointed_star: need at least 3 points.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension       = 2;
    static constexpr std::size_t point_count     = _N;
    static constexpr std::size_t vertex_count    = 2 * _N;
    static constexpr bool        is_shape        = true;
    static constexpr bool        is_2d           = true;
    static constexpr bool        is_3d           = false;
    static constexpr bool        is_star         = true;
    static constexpr bool        is_self_intersecting = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius_outer;
    value_type m_radius_inner;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    constexpr n_pointed_star() noexcept
        : m_center{},
          m_radius_outer(static_cast<value_type>(1)),
          m_radius_inner(static_cast<value_type>(0.4)),
          m_rotation(static_cast<value_type>(0))
    {
    }

    constexpr n_pointed_star(
        const point_type& _centre,
        value_type        _r_outer,
        value_type        _r_inner,
        value_type        _rot = static_cast<value_type>(0)
    ) noexcept
        : m_center(_centre),
          m_radius_outer(_r_outer),
          m_radius_inner(_r_inner),
          m_rotation(_rot)
    {
    }

    // ---- vertex enumeration -------------------------------------------------

    // vertex
    //   returns the _i-th vertex along the boundary (0 <= _i < 2N).
    // Even indices are outer points, odd indices are inner valleys.
    // Angular spacing is uniform at 2π / (2N).
    point_type
    vertex
    (
        std::size_t _i
    ) const noexcept
    {
        value_type                angle;
        value_type                radius;
        std::array<value_type, 2> v_cart;

        // uniform half-step around the circle
        angle = m_rotation +
                pi_v<value_type> *
                static_cast<value_type>(_i) /
                static_cast<value_type>(_N);

        // even = outer point, odd = inner valley
        radius = ((_i % 2) == 0) ? m_radius_outer
                                 : m_radius_inner;

        auto c_cart = _System::to_cartesian(m_center);

        v_cart[0] = c_cart[0] + radius * std::cos(angle);
        v_cart[1] = c_cart[1] + radius * std::sin(angle);

        return _System::from_cartesian(v_cart);
    }

    // outer_vertex
    //   returns the _i-th outer point (a star tip), 0 <= _i < N.
    point_type
    outer_vertex
    (
        std::size_t _i
    ) const noexcept
    {
        return vertex(2 * _i);
    }

    // inner_vertex
    //   returns the _i-th inner valley, 0 <= _i < N.
    point_type
    inner_vertex
    (
        std::size_t _i
    ) const noexcept
    {
        return vertex(2 * _i + 1);
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   standard ray-cast against the 2N-vertex simple boundary.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t  crossings;
        std::size_t  count;

        count     = 2 * _N;
        crossings = 0;

        auto p_cart = _System::to_cartesian(_p);

        // walk the simple boundary
        for (std::size_t i = 0, j = (count - 1); i < count; j = i++)
        {
            auto vi = _System::to_cartesian(vertex(i));
            auto vj = _System::to_cartesian(vertex(j));

            // standard ray-cast crossing predicate
            if ( ((vi[1] > p_cart[1]) != (vj[1] > p_cart[1])) &&
                 ( p_cart[0] < ( (vj[0] - vi[0]) *
                                  (p_cart[1] - vi[1]) /
                                  (vj[1] - vi[1]) +
                                  vi[0] ) ) )
            {
                ++crossings;
            }
        }

        return ((crossings % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   the star decomposes into 2N congruent triangles, each with
    // two sides of lengths R_outer and R_inner meeting at the centre
    // at angle π/N. Total area:
    //   A = N · R_outer · R_inner · sin(π / N)
    value_type
    closed_form_area
    () const noexcept
    {
        value_type theta;

        // angle between adjacent outer/inner radii
        theta = pi_v<value_type> /
                static_cast<value_type>(_N);

        return static_cast<value_type>(_N) *
               m_radius_outer * m_radius_inner *
               std::sin(theta);
    }

    // closed_form_perimeter
    //   the boundary has 2N edges of equal length L, where L is the
    // distance between an outer and an adjacent inner vertex. By the
    // law of cosines:
    //   L² = R_outer² + R_inner² - 2 R_outer R_inner cos(π/N)
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type theta;
        value_type chord_sq;

        theta = pi_v<value_type> /
                static_cast<value_type>(_N);

        // squared chord length between consecutive outer/inner vertices
        chord_sq = m_radius_outer * m_radius_outer +
                   m_radius_inner * m_radius_inner -
                   static_cast<value_type>(2) *
                   m_radius_outer * m_radius_inner *
                   std::cos(theta);

        return static_cast<value_type>(2) *
               static_cast<value_type>(_N) *
               std::sqrt(chord_sq);
    }

    // closed_form_centroid
    //   the centre by symmetry.
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// III.  HEXAGONAL STAR (Star of David, as a simple polygon)
// ============================================================================

// hexagonal_star
//   struct: the Star of David rendered as a simple 12-vertex
// non-convex polygon. Geometrically identical to n_pointed_star
// with N = 6 and the specific inner radius that produces the
// classic hexagram outline (R_inner = R_outer / sqrt(3)). Provided
// as a convenience that fixes the inner radius for users who just
// want "the" Star of David at a given size.
//
// For the geometric *compound* interpretation (two overlapping
// triangles), use hexagram instead - that's a star_polygon<.., 6, 2>
// whose boundary actually consists of two equilateral triangles.
template<typename _System = cartesian<2, double>>
struct hexagonal_star
{
    static_assert(_System::dimension == 2,
                  "hexagonal_star: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension    = 2;
    static constexpr std::size_t point_count  = 6;
    static constexpr std::size_t vertex_count = 12;
    static constexpr bool        is_shape     = true;
    static constexpr bool        is_2d        = true;
    static constexpr bool        is_3d        = false;
    static constexpr bool        is_star      = true;
    static constexpr bool        is_self_intersecting = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius_outer;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    constexpr hexagonal_star() noexcept
        : m_center{},
          m_radius_outer(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {
    }

    constexpr hexagonal_star(
        const point_type& _centre,
        value_type        _r_outer,
        value_type        _rot = static_cast<value_type>(0)
    ) noexcept
        : m_center(_centre),
          m_radius_outer(_r_outer),
          m_rotation(_rot)
    {
    }

    // ---- queries ------------------------------------------------------------

    // inner_radius
    //   the classic Star of David has R_inner = R_outer / sqrt(3).
    // Returned as a value for users who want to derive it.
    value_type
    inner_radius
    () const noexcept
    {
        return m_radius_outer /
               std::sqrt(static_cast<value_type>(3));
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   delegated to an internally-constructed n_pointed_star with
    // the fixed inner-radius ratio.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        n_pointed_star<_System, 6> as_simple{
            m_center,
            m_radius_outer,
            inner_radius(),
            m_rotation};

        return as_simple.contains(_p);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   substituting N = 6 and R_inner = R_outer / sqrt(3) into the
    // n_pointed_star formula:
    //   A = 6 · R · (R/√3) · sin(π/6)
    //     = 6 · R² / √3 · 1/2
    //     = √3 · R²
    value_type
    closed_form_area
    () const noexcept
    {
        return std::sqrt(static_cast<value_type>(3)) *
               m_radius_outer * m_radius_outer;
    }

    // closed_form_perimeter
    //   12 congruent edges; length derived from law of cosines with
    // the classic inner radius:
    //   L = R · √(1 + 1/3 - 2·(1/√3)·cos(π/6))
    //     = R · √(4/3 - 2/√3 · √3/2)
    //     = R · √(4/3 - 1)
    //     = R / √3
    // So perimeter = 12 R / √3 = 4 R √3.
    value_type
    closed_form_perimeter
    () const noexcept
    {
        return static_cast<value_type>(4) *
               std::sqrt(static_cast<value_type>(3)) *
               m_radius_outer;
    }

    // closed_form_centroid
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// IV.   NAMED ALIASES
// ============================================================================

// pentagram
//   type: the classic 5-point star polygon {5/2}.
template<typename _System = cartesian<2, double>>
using pentagram = star_polygon<_System, 5, 2>;

// hexagram
//   type: the {6/2} star polygon - a compound of two equilateral
// triangles. This is the "geometric" reading of the Star of David;
// for the polygon-outline reading, use hexagonal_star.
template<typename _System = cartesian<2, double>>
using hexagram = star_polygon<_System, 6, 2>;

// heptagram_7_2
//   type: the {7/2} heptagram (the narrower seven-pointed star).
template<typename _System = cartesian<2, double>>
using heptagram_7_2 = star_polygon<_System, 7, 2>;

// heptagram_7_3
//   type: the {7/3} heptagram (the wider seven-pointed star).
template<typename _System = cartesian<2, double>>
using heptagram_7_3 = star_polygon<_System, 7, 3>;

// octagram
//   type: the {8/3} octagram.
template<typename _System = cartesian<2, double>>
using octagram = star_polygon<_System, 8, 3>;

// enneagram_9_2
//   type: the {9/2} enneagram.
template<typename _System = cartesian<2, double>>
using enneagram_9_2 = star_polygon<_System, 9, 2>;

// enneagram_9_4
//   type: the {9/4} enneagram (the densest nine-pointed star).
template<typename _System = cartesian<2, double>>
using enneagram_9_4 = star_polygon<_System, 9, 4>;

// decagram
//   type: the {10/3} decagram.
template<typename _System = cartesian<2, double>>
using decagram = star_polygon<_System, 10, 3>;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_STAR_2D_
