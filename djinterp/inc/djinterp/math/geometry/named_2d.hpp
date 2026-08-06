/******************************************************************************
* djinterp [math]                                                  named_2d.hpp
*
* Named 2D shapes with closed-form measurements.
*   Each type here is a shorthand for a specific geometric configuration
* whose area, perimeter, and centroid have well-known analytic
* expressions. The measurement module (measure_2d.hpp) prefers these
* closed-form methods when present and falls back to numerical
* methods otherwise, so adding a named shape here also speeds up its
* measurement.
*
* All shapes satisfy the structural shape interface from
* geometry_common.hpp and live in any 2D coordinate system the user
* picks - centres, vertices, etc. are stored as runtime members of the
* point type from the user's coordinate system.
*
* PROVIDED TYPES:
*   circle<System>             - centre, radius
*   ellipse<System>            - centre, semi-major, semi-minor, rotation
*   rectangle<System>          - axis-aligned: corner, width, height
*   square<System>             - axis-aligned: corner, side
*   triangle<System>           - three vertices
*   regular_polygon<System, N> - centre, circumradius, rotation
*   annulus<System>            - centre, inner and outer radii
*   circular_sector<System>    - centre, radius, start and sweep angles
*   circular_segment<System>   - centre, radius, sweep angle
*
* path:      /inc/djinterp/math/geometry/named_2d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_NAMED_2D_
#define DJINTERP_MATH_GEOMETRY_NAMED_2D_ 1

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
// I.    CIRCLE
// ============================================================================

// circle
//   struct: a 2D circle with centre and radius. Closed-form area
// π r², perimeter 2π r, centroid == centre.
template<typename _System = cartesian<2, double>>
struct circle
{
    static_assert(_System::dimension == 2,
                  "circle: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;

    // ---- construction -------------------------------------------------------

    constexpr circle() noexcept
        : m_center{},
          m_radius(static_cast<value_type>(1))
    {
    }

    constexpr circle(
        const point_type& _center_point,
        value_type        _r
    ) noexcept
        : m_center(_center_point),
          m_radius(_r)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   true when Cartesian distance from _p to centre ≤ radius.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        return ( cartesian_distance<_System>(m_center, _p) <=
                 m_radius );
    }

    // ---- closed-form measurements -------------------------------------------

    constexpr value_type
    closed_form_area
    () const noexcept
    {
        return pi_v<value_type> * m_radius * m_radius;
    }

    constexpr value_type
    closed_form_perimeter
    () const noexcept
    {
        return two_pi_v<value_type> * m_radius;
    }

    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// II.   ELLIPSE
// ============================================================================

// ellipse
//   struct: a 2D ellipse with semi-major and semi-minor axes and a
// rotation angle (in radians, ccw from x-axis). Area is exact
// (π·a·b); perimeter uses Ramanujan's second approximation, which is
// accurate to better than 1 ppm for most realistic eccentricities.
template<typename _System = cartesian<2, double>>
struct ellipse
{
    static_assert(_System::dimension == 2,
                  "ellipse: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_semi_major;
    value_type m_semi_minor;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    constexpr ellipse() noexcept
        : m_center{},
          m_semi_major(static_cast<value_type>(1)),
          m_semi_minor(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {
    }

    constexpr ellipse(
        const point_type& _center_point,
        value_type        _a,
        value_type        _b,
        value_type        _rot = static_cast<value_type>(0)
    ) noexcept
        : m_center(_center_point),
          m_semi_major(_a),
          m_semi_minor(_b),
          m_rotation(_rot)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   inverse-rotates the test point into the ellipse's body frame
    // and checks the canonical (x/a)² + (y/b)² ≤ 1 predicate.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        value_type c_rot;
        value_type s_rot;
        value_type dx;
        value_type dy;
        value_type local_x;
        value_type local_y;
        value_type term_a;
        value_type term_b;

        auto p_cart = _System::to_cartesian(_p);
        auto c_cart = _System::to_cartesian(m_center);

        // displacement from centre, in cartesian
        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];

        // rotate into the ellipse's local frame (negate rotation)
        c_rot = std::cos(-m_rotation);
        s_rot = std::sin(-m_rotation);

        local_x = c_rot * dx - s_rot * dy;
        local_y = s_rot * dx + c_rot * dy;

        term_a = local_x / m_semi_major;
        term_b = local_y / m_semi_minor;

        return ( (term_a * term_a + term_b * term_b) <=
                 static_cast<value_type>(1) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   π · a · b
    constexpr value_type
    closed_form_area
    () const noexcept
    {
        return pi_v<value_type> * m_semi_major * m_semi_minor;
    }

    // closed_form_perimeter
    //   Ramanujan's second approximation:
    //     P ≈ π (a+b) (1 + 3h / (10 + √(4 - 3h)))
    //   with h = ((a - b) / (a + b))².
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type a_plus_b;
        value_type a_minus_b;
        value_type h;
        value_type inner;

        a_plus_b  = m_semi_major + m_semi_minor;
        a_minus_b = m_semi_major - m_semi_minor;
        h         = (a_minus_b * a_minus_b) /
                    (a_plus_b * a_plus_b);

        // Ramanujan's correction factor
        inner = static_cast<value_type>(1) +
                ( static_cast<value_type>(3) * h ) /
                ( static_cast<value_type>(10) +
                  std::sqrt(static_cast<value_type>(4) -
                            static_cast<value_type>(3) * h) );

        return pi_v<value_type> * a_plus_b * inner;
    }

    // closed_form_centroid
    //   centre of the ellipse.
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// III.  RECTANGLE (axis-aligned)
// ============================================================================

// rectangle
//   struct: an axis-aligned rectangle. m_corner is the lower-left
// (minimum coordinate) corner in Cartesian. For non-axis-aligned
// rectangles, use polygon<System, 4> or rotate the user's coordinate
// system.
template<typename _System = cartesian<2, double>>
struct rectangle
{
    static_assert(_System::dimension == 2,
                  "rectangle: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_corner;
    value_type m_width;
    value_type m_height;

    // ---- construction -------------------------------------------------------

    constexpr rectangle() noexcept
        : m_corner{},
          m_width(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1))
    {
    }

    constexpr rectangle(
        const point_type& _lower_left,
        value_type        _w,
        value_type        _h
    ) noexcept
        : m_corner(_lower_left),
          m_width(_w),
          m_height(_h)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   axis-aligned bounds test in Cartesian.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto p_cart = _System::to_cartesian(_p);
        auto c_cart = _System::to_cartesian(m_corner);

        return ( (p_cart[0] >=  c_cart[0]) &&
                 (p_cart[0] <= (c_cart[0] + m_width))  &&
                 (p_cart[1] >=  c_cart[1]) &&
                 (p_cart[1] <= (c_cart[1] + m_height)) );
    }

    // ---- closed-form measurements -------------------------------------------

    constexpr value_type
    closed_form_area
    () const noexcept
    {
        return m_width * m_height;
    }

    constexpr value_type
    closed_form_perimeter
    () const noexcept
    {
        return static_cast<value_type>(2) * (m_width + m_height);
    }

    // closed_form_centroid
    //   geometric centre = corner + (w/2, h/2).
    point_type
    closed_form_centroid
    () const noexcept
    {
        std::array<value_type, 2> c_cart;

        auto corner_cart = _System::to_cartesian(m_corner);

        c_cart[0] = corner_cart[0] +
                    m_width  / static_cast<value_type>(2);
        c_cart[1] = corner_cart[1] +
                    m_height / static_cast<value_type>(2);

        return _System::from_cartesian(c_cart);
    }
};


// ============================================================================
// IV.   SQUARE
// ============================================================================

// square
//   struct: an axis-aligned square. Stored as corner + side length;
// equivalent to a rectangle with width == height, but more
// self-documenting.
template<typename _System = cartesian<2, double>>
struct square
{
    static_assert(_System::dimension == 2,
                  "square: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_corner;
    value_type m_side;

    // ---- construction -------------------------------------------------------

    constexpr square() noexcept
        : m_corner{},
          m_side(static_cast<value_type>(1))
    {
    }

    constexpr square(
        const point_type& _lower_left,
        value_type        _s
    ) noexcept
        : m_corner(_lower_left),
          m_side(_s)
    {
    }

    // ---- structural interface -----------------------------------------------

    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto p_cart = _System::to_cartesian(_p);
        auto c_cart = _System::to_cartesian(m_corner);

        return ( (p_cart[0] >=  c_cart[0]) &&
                 (p_cart[0] <= (c_cart[0] + m_side)) &&
                 (p_cart[1] >=  c_cart[1]) &&
                 (p_cart[1] <= (c_cart[1] + m_side)) );
    }

    // ---- closed-form measurements -------------------------------------------

    constexpr value_type
    closed_form_area
    () const noexcept
    {
        return m_side * m_side;
    }

    constexpr value_type
    closed_form_perimeter
    () const noexcept
    {
        return static_cast<value_type>(4) * m_side;
    }

    point_type
    closed_form_centroid
    () const noexcept
    {
        std::array<value_type, 2> c_cart;

        auto corner_cart = _System::to_cartesian(m_corner);

        c_cart[0] = corner_cart[0] +
                    m_side / static_cast<value_type>(2);
        c_cart[1] = corner_cart[1] +
                    m_side / static_cast<value_type>(2);

        return _System::from_cartesian(c_cart);
    }
};


// ============================================================================
// V.    TRIANGLE
// ============================================================================

// triangle
//   struct: a triangle defined by three vertices. Area uses the
// cross-product formula in Cartesian; perimeter sums the three side
// lengths; centroid is the vertex average.
template<typename _System = cartesian<2, double>>
struct triangle
{
    static_assert(_System::dimension == 2,
                  "triangle: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_v0;
    point_type m_v1;
    point_type m_v2;

    // ---- construction -------------------------------------------------------

    constexpr triangle() noexcept
        : m_v0{},
          m_v1{},
          m_v2{}
    {
    }

    constexpr triangle(
        const point_type& _vertex_0,
        const point_type& _vertex_1,
        const point_type& _vertex_2
    ) noexcept
        : m_v0(_vertex_0),
          m_v1(_vertex_1),
          m_v2(_vertex_2)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   barycentric / same-sign cross-product test in Cartesian.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        value_type d1;
        value_type d2;
        value_type d3;
        bool       has_neg;
        bool       has_pos;

        auto p  = _System::to_cartesian(_p);
        auto a  = _System::to_cartesian(m_v0);
        auto b  = _System::to_cartesian(m_v1);
        auto c  = _System::to_cartesian(m_v2);

        // signed edge tests
        d1 = (p[0] - b[0]) * (a[1] - b[1]) -
             (a[0] - b[0]) * (p[1] - b[1]);
        d2 = (p[0] - c[0]) * (b[1] - c[1]) -
             (b[0] - c[0]) * (p[1] - c[1]);
        d3 = (p[0] - a[0]) * (c[1] - a[1]) -
             (c[0] - a[0]) * (p[1] - a[1]);

        has_neg = (d1 < static_cast<value_type>(0)) ||
                  (d2 < static_cast<value_type>(0)) ||
                  (d3 < static_cast<value_type>(0));
        has_pos = (d1 > static_cast<value_type>(0)) ||
                  (d2 > static_cast<value_type>(0)) ||
                  (d3 > static_cast<value_type>(0));

        // inside iff all three signs agree (or zero)
        return !(has_neg && has_pos);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   |((v1 - v0) × (v2 - v0))| / 2.
    value_type
    closed_form_area
    () const noexcept
    {
        value_type cross;

        auto a = _System::to_cartesian(m_v0);
        auto b = _System::to_cartesian(m_v1);
        auto c = _System::to_cartesian(m_v2);

        cross = (b[0] - a[0]) * (c[1] - a[1]) -
                (c[0] - a[0]) * (b[1] - a[1]);

        // half absolute cross product
        if (cross < static_cast<value_type>(0))
        {
            cross = -cross;
        }

        return cross / static_cast<value_type>(2);
    }

    // closed_form_perimeter
    //   sum of side lengths.
    value_type
    closed_form_perimeter
    () const noexcept
    {
        return ( cartesian_distance<_System>(m_v0, m_v1) +
                 cartesian_distance<_System>(m_v1, m_v2) +
                 cartesian_distance<_System>(m_v2, m_v0) );
    }

    // closed_form_centroid
    //   arithmetic mean of the three vertices.
    point_type
    closed_form_centroid
    () const noexcept
    {
        std::array<value_type, 2> c_cart;

        auto a = _System::to_cartesian(m_v0);
        auto b = _System::to_cartesian(m_v1);
        auto c = _System::to_cartesian(m_v2);

        c_cart[0] = (a[0] + b[0] + c[0]) /
                    static_cast<value_type>(3);
        c_cart[1] = (a[1] + b[1] + c[1]) /
                    static_cast<value_type>(3);

        return _System::from_cartesian(c_cart);
    }
};


// ============================================================================
// VI.   REGULAR POLYGON
// ============================================================================

// regular_polygon
//   struct: an _N-sided regular polygon defined by centre,
// circumradius (distance from centre to vertex), and rotation
// (angular offset of vertex 0 from the positive x-axis).
//
//   Area      = (1/2) N r² sin(2π/N)
//   Perimeter = 2 N r sin(π/N)
//   Centroid  = centre
template<typename    _System = cartesian<2, double>,
         std::size_t _N      = 6>
struct regular_polygon
{
    static_assert(_System::dimension == 2,
                  "regular_polygon: 2D coordinate systems only.");
    static_assert(_N >= 3,
                  "regular_polygon: need at least 3 sides.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr std::size_t sides     = _N;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_circumradius;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    constexpr regular_polygon() noexcept
        : m_center{},
          m_circumradius(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {
    }

    constexpr regular_polygon(
        const point_type& _center_point,
        value_type        _r,
        value_type        _rot = static_cast<value_type>(0)
    ) noexcept
        : m_center(_center_point),
          m_circumradius(_r),
          m_rotation(_rot)
    {
    }

    // ---- queries ------------------------------------------------------------

    // vertex
    //   returns the _i-th vertex (0 ≤ _i < _N) in the user's
    // coordinate system.
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

    // ---- structural interface -----------------------------------------------

    // contains
    //   the point lies inside if its distance to the centre is below
    // the apothem along the corresponding radial direction. We use a
    // simple ray-cast over the vertices for robustness.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t crossings;

        auto p_cart = _System::to_cartesian(_p);
        crossings   = 0;

        // walk the implicit vertex list
        for (std::size_t i = 0, j = (_N - 1); i < _N; j = i++)
        {
            auto vi = _System::to_cartesian(vertex(i));
            auto vj = _System::to_cartesian(vertex(j));

            // standard ray-cast crossing test
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
    //   (1/2) N r² sin(2π/N).
    value_type
    closed_form_area
    () const noexcept
    {
        value_type theta;

        theta = two_pi_v<value_type> /
                static_cast<value_type>(_N);

        return static_cast<value_type>(0.5) *
               static_cast<value_type>(_N) *
               m_circumradius * m_circumradius *
               std::sin(theta);
    }

    // closed_form_perimeter
    //   2 N r sin(π/N).
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type theta;

        theta = pi_v<value_type> /
                static_cast<value_type>(_N);

        return static_cast<value_type>(2) *
               static_cast<value_type>(_N) *
               m_circumradius * std::sin(theta);
    }

    // closed_form_centroid
    //   centre.
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }
};


// ============================================================================
// VII.  ANNULUS
// ============================================================================

// annulus
//   struct: the region between two concentric circles of radii
// r_inner ≤ r_outer.
//
//   Area      = π (r_outer² - r_inner²)
//   Perimeter = 2π (r_outer + r_inner)         [both boundary curves]
//   Centroid  = centre
template<typename _System = cartesian<2, double>>
struct annulus
{
    static_assert(_System::dimension == 2,
                  "annulus: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius_inner;
    value_type m_radius_outer;

    // ---- construction -------------------------------------------------------

    constexpr annulus() noexcept
        : m_center{},
          m_radius_inner(static_cast<value_type>(0)),
          m_radius_outer(static_cast<value_type>(1))
    {
    }

    constexpr annulus(
        const point_type& _center_point,
        value_type        _r_inner,
        value_type        _r_outer
    ) noexcept
        : m_center(_center_point),
          m_radius_inner(_r_inner),
          m_radius_outer(_r_outer)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   r_inner ≤ distance(p, centre) ≤ r_outer.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        value_type d;

        d = cartesian_distance<_System>(m_center, _p);

        return ( (d >= m_radius_inner) &&
                 (d <= m_radius_outer) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   π (r_outer² - r_inner²).
    constexpr value_type
    closed_form_area
    () const noexcept
    {
        return pi_v<value_type> *
               ( m_radius_outer * m_radius_outer -
                 m_radius_inner * m_radius_inner );
    }

    // closed_form_perimeter
    //   total length of both boundary circles.
    constexpr value_type
    closed_form_perimeter
    () const noexcept
    {
        return two_pi_v<value_type> *
               (m_radius_outer + m_radius_inner);
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
// VIII. CIRCULAR SECTOR
// ============================================================================

// circular_sector
//   struct: the "pie slice" region of a disk between two radii.
// Sweep angle is the absolute angular width (always positive).
//
//   Area      = (1/2) r² · sweep
//   Perimeter = 2 r + r · sweep                [two radii + arc]
template<typename _System = cartesian<2, double>>
struct circular_sector
{
    static_assert(_System::dimension == 2,
                  "circular_sector: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;
    value_type m_start_angle;
    value_type m_sweep;

    // ---- construction -------------------------------------------------------

    constexpr circular_sector() noexcept
        : m_center{},
          m_radius(static_cast<value_type>(1)),
          m_start_angle(static_cast<value_type>(0)),
          m_sweep(half_pi_v<value_type>)
    {
    }

    constexpr circular_sector(
        const point_type& _center_point,
        value_type        _r,
        value_type        _start_a,
        value_type        _sweep_a
    ) noexcept
        : m_center(_center_point),
          m_radius(_r),
          m_start_angle(_start_a),
          m_sweep(_sweep_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   inside the disk AND within the angular wedge measured from
    // the start angle.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        value_type dx;
        value_type dy;
        value_type r;
        value_type theta;
        value_type relative;
        value_type sweep_abs;

        auto p_cart = _System::to_cartesian(_p);
        auto c_cart = _System::to_cartesian(m_center);

        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];
        r  = std::sqrt(dx * dx + dy * dy);

        // outside the disk fails immediately
        if (r > m_radius)
        {
            return false;
        }

        theta    = std::atan2(dy, dx);
        relative = theta - m_start_angle;

        // wrap into [0, 2π)
        while (relative < static_cast<value_type>(0))
        {
            relative += two_pi_v<value_type>;
        }

        while (relative >= two_pi_v<value_type>)
        {
            relative -= two_pi_v<value_type>;
        }

        sweep_abs = m_sweep;

        if (sweep_abs < static_cast<value_type>(0))
        {
            sweep_abs = -sweep_abs;
        }

        return (relative <= sweep_abs);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   (1/2) r² · |sweep|.
    value_type
    closed_form_area
    () const noexcept
    {
        value_type s;

        s = m_sweep;

        // use absolute sweep
        if (s < static_cast<value_type>(0))
        {
            s = -s;
        }

        return static_cast<value_type>(0.5) *
               m_radius * m_radius * s;
    }

    // closed_form_perimeter
    //   2 r + r · |sweep| (two radial sides + the arc).
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type s;

        s = m_sweep;

        if (s < static_cast<value_type>(0))
        {
            s = -s;
        }

        return static_cast<value_type>(2) * m_radius +
               m_radius * s;
    }
};


// ============================================================================
// IX.   CIRCULAR SEGMENT
// ============================================================================

// circular_segment
//   struct: the chord-bounded region of a disk subtended by an angle
// _sweep at the centre. Note: this is the SMALLER region when
// sweep < π.
//
//   Area      = (1/2) r² (sweep - sin(sweep))
//   Perimeter = arc + chord
//             = r·sweep + 2 r sin(sweep / 2)
template<typename _System = cartesian<2, double>>
struct circular_segment
{
    static_assert(_System::dimension == 2,
                  "circular_segment: 2D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 2;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = true;
    static constexpr bool        is_3d     = false;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;
    value_type m_start_angle;
    value_type m_sweep;

    // ---- construction -------------------------------------------------------

    constexpr circular_segment() noexcept
        : m_center{},
          m_radius(static_cast<value_type>(1)),
          m_start_angle(static_cast<value_type>(0)),
          m_sweep(pi_v<value_type>)
    {
    }

    constexpr circular_segment(
        const point_type& _center_point,
        value_type        _r,
        value_type        _start_a,
        value_type        _sweep_a
    ) noexcept
        : m_center(_center_point),
          m_radius(_r),
          m_start_angle(_start_a),
          m_sweep(_sweep_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   inside the sector AND on the far side of the chord. We test
    // by checking that the point is inside the disk (radial test),
    // inside the angular wedge, and on the "outer" side of the chord
    // line - i.e. at a larger projection along the bisector than the
    // chord midpoint.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        value_type dx;
        value_type dy;
        value_type r;
        value_type chord_distance;
        value_type sweep_abs;

        auto p_cart = _System::to_cartesian(_p);
        auto c_cart = _System::to_cartesian(m_center);

        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];
        r  = std::sqrt(dx * dx + dy * dy);

        // outside disk - cannot be in the segment
        if (r > m_radius)
        {
            return false;
        }

        sweep_abs = m_sweep;

        if (sweep_abs < static_cast<value_type>(0))
        {
            sweep_abs = -sweep_abs;
        }

        // perpendicular distance from chord:
        //   sagitta = r - r cos(sweep/2)
        //   point lies in segment when its projection along the
        //   bisector exceeds r cos(sweep/2).
        value_type bisector = m_start_angle + sweep_abs /
                              static_cast<value_type>(2);
        value_type along    = dx * std::cos(bisector) +
                              dy * std::sin(bisector);

        chord_distance = m_radius *
                         std::cos(sweep_abs /
                                  static_cast<value_type>(2));

        return (along >= chord_distance);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   (1/2) r² (sweep - sin(sweep)).
    value_type
    closed_form_area
    () const noexcept
    {
        value_type s;

        s = m_sweep;

        if (s < static_cast<value_type>(0))
        {
            s = -s;
        }

        return static_cast<value_type>(0.5) *
               m_radius * m_radius *
               (s - std::sin(s));
    }

    // closed_form_perimeter
    //   r·sweep + 2 r sin(sweep / 2).
    value_type
    closed_form_perimeter
    () const noexcept
    {
        value_type s;
        value_type arc_len;
        value_type chord_len;

        s = m_sweep;

        if (s < static_cast<value_type>(0))
        {
            s = -s;
        }

        arc_len   = m_radius * s;
        chord_len = static_cast<value_type>(2) * m_radius *
                    std::sin(s / static_cast<value_type>(2));

        return arc_len + chord_len;
    }
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_NAMED_2D_
