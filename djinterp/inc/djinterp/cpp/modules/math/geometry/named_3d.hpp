/*******************************************************************************
* djinterp [cpp/math]                                               named_3d.hpp
*
* Named 3D solids with closed-form measurements.
*   Each type here is a shorthand for a specific 3D configuration whose
* volume, surface area, and centroid have well-known analytic formulas.
* The measurement module (measure_3d.hpp) prefers these closed-form
* methods when present and falls back to numerical procedures otherwise,
* so adding a named solid here also speeds up its measurement.
*
* All shapes satisfy the structural shape interface from
* geometry_common.hpp and live in any 3D coordinate system the user
* picks - centres, dimensions, etc. are stored as runtime members of
* the point type / value type from the user's coordinate system.
*
* PROVIDED TYPES:
*   sphere<System>                - centre, radius
*   ellipsoid<System>             - centre, three semi-axes
*   box<System>                   - axis-aligned: corner, extents
*   cube<System>                  - axis-aligned: corner, side
*   cylinder<System>              - base centre on +z axis, radius, height
*   cone<System>                  - apex / base configuration on +z axis
*   frustum<System>               - truncated cone, two radii + height
*   torus<System>                 - centre, major and minor radii
*   hemisphere<System>            - sphere clipped at the z = centre plane
*   spherical_cap<System>         - sphere clipped by a horizontal plane
*   regular_prism<System, N>      - N-sided regular polygon extruded along z
*   regular_pyramid<System, N>    - N-sided regular base, apex above centre
*
* AXIS CONVENTION:
*   Solids with a distinguished axis (cylinder, cone, frustum, prism,
*   pyramid, hemisphere, spherical_cap) are oriented along the
*   Cartesian +z direction. For other orientations, rotate the user's
*   coordinate system or construct the solid via shape_from_surfaces.
*
* LEVELS:
*   Everything compiles from C++98. Points are linalg::vectors; the closed
* forms are the formulas they were, on the C library's functions through
* <cmath>, as before; pi_v and its kin are pi_of<T>() and its kin.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/named_3d.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SPHERE
    ------
2.  ELLIPSOID
    ---------
3.  BOX (AXIS-ALIGNED)
    ------------------
4.  CUBE
    ----
5.  CYLINDER (RIGHT CIRCULAR, +Z AXIS)
    ----------------------------------
6.  CONE (RIGHT CIRCULAR, +Z AXIS, APEX UP)
    ---------------------------------------
7.  FRUSTUM (TRUNCATED CONE, +Z AXIS)
    ---------------------------------
8.  TORUS
    -----
9.  HEMISPHERE
    ----------
10. SPHERICAL CAP
    -------------
11. REGULAR PRISM
    -------------
12. REGULAR PYRAMID
    ---------------
*/

#ifndef DJINTERP_MATH_GEOMETRY_NAMED_3D_HPP
#define DJINTERP_MATH_GEOMETRY_NAMED_3D_HPP 1

// std
#include <cmath>                                  // std::sqrt, std::pow,
                                                  // std::sin, std::cos
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // cartesian_distance,
                                                  // pi_of


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  SPHERE
//==============================================================================

// sphere
//   struct: a 3D ball with centre and radius. Closed-form volume
// (4/3)πr³, surface area 4πr², centroid == centre.
template<typename System = cartesian<3, double> >
struct sphere
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "sphere: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR sphere() D_NOEXCEPT
        : m_center(),
          m_radius(static_cast<value_type>(1))
    {}

    D_CONSTEXPR sphere(
        const point_type& _centre,
        value_type        _r
    ) D_NOEXCEPT
        : m_center(_centre),
          m_radius(_r)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   true when Cartesian distance from _p to centre ≤ radius.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        return ( cartesian_distance<System>(m_center, _p) <=
                 m_radius );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   (4/3) π r³.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return ( static_cast<value_type>(4) / static_cast<value_type>(3) ) *
               pi_of<value_type>() *
               m_radius * m_radius * m_radius;
    }

    // closed_form_surface_area
    //   4 π r².
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(4) *
               pi_of<value_type>() *
               m_radius * m_radius;
    }

    // closed_form_centroid
    //   centre.
    D_CONSTEXPR point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        return m_center;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t sphere<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool sphere<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool sphere<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool sphere<System>::is_3d;


//==============================================================================
// 2.  ELLIPSOID
//==============================================================================

// ellipsoid
//   struct: a 3D ellipsoid with three semi-axes (a, b, c) aligned to
// the Cartesian x, y, z axes. Volume is exact, (4/3)πabc. Surface
// area has no elementary closed form; we use Knud Thomsen's
// approximation:
//   S ≈ 4π ((a^p b^p + a^p c^p + b^p c^p) / 3)^(1/p),  p = 1.6075
// which is accurate to better than 1.061% for any ellipsoid.
template<typename System = cartesian<3, double> >
struct ellipsoid
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "ellipsoid: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_a;
    value_type m_b;
    value_type m_c;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR ellipsoid() D_NOEXCEPT
        : m_center(),
          m_a(static_cast<value_type>(1)),
          m_b(static_cast<value_type>(1)),
          m_c(static_cast<value_type>(1))
    {}

    D_CONSTEXPR ellipsoid(
        const point_type& _centre,
        value_type        _semi_x,
        value_type        _semi_y,
        value_type        _semi_z
    ) D_NOEXCEPT
        : m_center(_centre),
          m_a(_semi_x),
          m_b(_semi_y),
          m_c(_semi_z)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   (x/a)² + (y/b)² + (z/c)² ≤ 1 in Cartesian, after translating
    // so the centre is at the origin.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type tx;
        value_type ty;
        value_type tz;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_center);

        // displacement from centre, scaled by each semi-axis
        tx = (p_cart[0] - c_cart[0]) / m_a;
        ty = (p_cart[1] - c_cart[1]) / m_b;
        tz = (p_cart[2] - c_cart[2]) / m_c;

        return ( (tx * tx + ty * ty + tz * tz) <=
                 static_cast<value_type>(1) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   (4/3) π a b c.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return ( static_cast<value_type>(4) / static_cast<value_type>(3) ) *
               pi_of<value_type>() *
               m_a * m_b * m_c;
    }

    // closed_form_surface_area
    //   Knud Thomsen approximation with p = 1.6075.
    value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        const value_type p = static_cast<value_type>(1.6075L);

        value_type ap;
        value_type bp;
        value_type cp;
        value_type inner;

        ap = std::pow(m_a, p);
        bp = std::pow(m_b, p);
        cp = std::pow(m_c, p);

        // mean of pairwise products
        inner = (ap * bp + ap * cp + bp * cp) /
                static_cast<value_type>(3);

        return static_cast<value_type>(4) *
               pi_of<value_type>() *
               std::pow(inner,
                        static_cast<value_type>(1) / p);
    }

    // closed_form_centroid
    D_CONSTEXPR point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        return m_center;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t ellipsoid<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool ellipsoid<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool ellipsoid<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool ellipsoid<System>::is_3d;


//==============================================================================
// 3.  BOX (AXIS-ALIGNED)
//==============================================================================

// box
//   struct: an axis-aligned rectangular cuboid. m_corner is the
// minimum-coordinate corner in Cartesian; the opposite corner is
// m_corner + (width, height, depth). For rotated boxes, rotate the
// user's coordinate system or use shape_from_surfaces.
template<typename System = cartesian<3, double> >
struct box
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "box: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_corner;
    value_type m_width;
    value_type m_height;
    value_type m_depth;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR box() D_NOEXCEPT
        : m_corner(),
          m_width(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1)),
          m_depth(static_cast<value_type>(1))
    {}

    D_CONSTEXPR box(
        const point_type& _min_corner,
        value_type        _w,
        value_type        _h,
        value_type        _d
    ) D_NOEXCEPT
        : m_corner(_min_corner),
          m_width(_w),
          m_height(_h),
          m_depth(_d)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   axis-aligned bounds test in Cartesian.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_corner);

        return ( (p_cart[0] >=  c_cart[0]) &&
                 (p_cart[0] <= (c_cart[0] + m_width))  &&
                 (p_cart[1] >=  c_cart[1]) &&
                 (p_cart[1] <= (c_cart[1] + m_height)) &&
                 (p_cart[2] >=  c_cart[2]) &&
                 (p_cart[2] <= (c_cart[2] + m_depth)) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   w · h · d.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return m_width * m_height * m_depth;
    }

    // closed_form_surface_area
    //   2(wh + wd + hd).
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               ( m_width  * m_height +
                 m_width  * m_depth  +
                 m_height * m_depth );
    }

    // closed_form_centroid
    //   geometric centre = corner + (w/2, h/2, d/2).
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type corner_cart = System::to_cartesian(m_corner);

        c_cart[0] = corner_cart[0] +
                    m_width  / static_cast<value_type>(2);
        c_cart[1] = corner_cart[1] +
                    m_height / static_cast<value_type>(2);
        c_cart[2] = corner_cart[2] +
                    m_depth  / static_cast<value_type>(2);

        return System::from_cartesian(c_cart);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t box<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool box<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool box<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool box<System>::is_3d;


//==============================================================================
// 4.  CUBE
//==============================================================================

// cube
//   struct: an axis-aligned cube. Stored as corner + side length;
// equivalent to a box with width == height == depth, but more
// self-documenting.
template<typename System = cartesian<3, double> >
struct cube
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "cube: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_corner;
    value_type m_side;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR cube() D_NOEXCEPT
        : m_corner(),
          m_side(static_cast<value_type>(1))
    {}

    D_CONSTEXPR cube(
        const point_type& _min_corner,
        value_type        _s
    ) D_NOEXCEPT
        : m_corner(_min_corner),
          m_side(_s)
    {}

    // ---- structural interface -----------------------------------------------

    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_corner);

        return ( (p_cart[0] >=  c_cart[0]) &&
                 (p_cart[0] <= (c_cart[0] + m_side)) &&
                 (p_cart[1] >=  c_cart[1]) &&
                 (p_cart[1] <= (c_cart[1] + m_side)) &&
                 (p_cart[2] >=  c_cart[2]) &&
                 (p_cart[2] <= (c_cart[2] + m_side)) );
    }

    // ---- closed-form measurements -------------------------------------------

    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return m_side * m_side * m_side;
    }

    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(6) * m_side * m_side;
    }

    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type corner_cart = System::to_cartesian(m_corner);

        c_cart[0] = corner_cart[0] +
                    m_side / static_cast<value_type>(2);
        c_cart[1] = corner_cart[1] +
                    m_side / static_cast<value_type>(2);
        c_cart[2] = corner_cart[2] +
                    m_side / static_cast<value_type>(2);

        return System::from_cartesian(c_cart);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t cube<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool cube<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool cube<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool cube<System>::is_3d;


//==============================================================================
// 5.  CYLINDER (RIGHT CIRCULAR, +Z AXIS)
//==============================================================================

// cylinder
//   struct: a right circular cylinder oriented along +z. Base is a
// disk centred at m_base, top is a disk centred at m_base + (0, 0, h).
//
//   Volume        = π r² h
//   Surface area  = 2π r² + 2π r h   (two caps + lateral)
//   Lateral area  = 2π r h           (just the curved side)
//   Centroid      = base_centre + (0, 0, h/2)
template<typename System = cartesian<3, double> >
struct cylinder
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "cylinder: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_base;
    value_type m_radius;
    value_type m_height;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR cylinder() D_NOEXCEPT
        : m_base(),
          m_radius(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1))
    {}

    D_CONSTEXPR cylinder(
        const point_type& _base_centre,
        value_type        _r,
        value_type        _h
    ) D_NOEXCEPT
        : m_base(_base_centre),
          m_radius(_r),
          m_height(_h)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   height test plus radial distance test in Cartesian.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dz;
        value_type dx;
        value_type dy;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type b_cart = System::to_cartesian(m_base);

        dz = p_cart[2] - b_cart[2];

        // outside the height range fails
        if ( (dz < static_cast<value_type>(0)) ||
             (dz > m_height) )
        {
            return false;
        }

        // radial test in xy-plane
        dx = p_cart[0] - b_cart[0];
        dy = p_cart[1] - b_cart[1];

        return ( (dx * dx + dy * dy) <=
                 (m_radius * m_radius) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   π r² h.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return pi_of<value_type>() *
               m_radius * m_radius *
               m_height;
    }

    // closed_form_surface_area
    //   2π r² (caps) + 2π r h (lateral).
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() *
               m_radius *
               (m_radius + m_height);
    }

    // lateral_area
    //   curved side only: 2π r h.
    D_CONSTEXPR value_type
    lateral_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() *
               m_radius *
               m_height;
    }

    // closed_form_centroid
    //   on the cylinder axis, half the height above the base.
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type b_cart = System::to_cartesian(m_base);

        c_cart[0] = b_cart[0];
        c_cart[1] = b_cart[1];
        c_cart[2] = b_cart[2] +
                    m_height / static_cast<value_type>(2);

        return System::from_cartesian(c_cart);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t cylinder<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool cylinder<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool cylinder<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool cylinder<System>::is_3d;


//==============================================================================
// 6.  CONE (RIGHT CIRCULAR, +Z AXIS, APEX UP)
//==============================================================================

// cone
//   struct: a right circular cone, base centred at m_base with radius
// m_radius, apex at m_base + (0, 0, m_height).
//
//   Volume        = (1/3) π r² h
//   Slant height  = √(r² + h²)
//   Surface area  = π r² + π r ℓ   (base + lateral)
//   Lateral area  = π r ℓ
//   Centroid      = base_centre + (0, 0, h/4)
template<typename System = cartesian<3, double> >
struct cone
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "cone: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_base;
    value_type m_radius;
    value_type m_height;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR cone() D_NOEXCEPT
        : m_base(),
          m_radius(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1))
    {}

    D_CONSTEXPR cone(
        const point_type& _base_centre,
        value_type        _r,
        value_type        _h
    ) D_NOEXCEPT
        : m_base(_base_centre),
          m_radius(_r),
          m_height(_h)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   in the cone iff 0 ≤ dz ≤ h and √(dx²+dy²) ≤ r(1 - dz/h).
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dz;
        value_type dx;
        value_type dy;
        value_type r_at_z;
        value_type rho_sq;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type b_cart = System::to_cartesian(m_base);

        dz = p_cart[2] - b_cart[2];

        // outside height range
        if ( (dz < static_cast<value_type>(0)) ||
             (dz > m_height) )
        {
            return false;
        }

        // radius shrinks linearly to zero at the apex
        r_at_z = m_radius *
                 (static_cast<value_type>(1) - dz / m_height);

        dx = p_cart[0] - b_cart[0];
        dy = p_cart[1] - b_cart[1];
        rho_sq = dx * dx + dy * dy;

        return (rho_sq <= (r_at_z * r_at_z));
    }

    // ---- closed-form measurements -------------------------------------------

    // slant_height
    //   ℓ = √(r² + h²).
    value_type
    slant_height() const D_NOEXCEPT
    {
        return std::sqrt(m_radius * m_radius +
                          m_height * m_height);
    }

    // closed_form_volume
    //   (1/3) π r² h.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return pi_of<value_type>() *
               m_radius * m_radius *
               m_height /
               static_cast<value_type>(3);
    }

    // closed_form_surface_area
    //   π r² + π r ℓ.
    value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        value_type slant;

        slant = slant_height();

        return pi_of<value_type>() * m_radius *
               (m_radius + slant);
    }

    // lateral_area
    //   π r ℓ (the curved side only).
    value_type
    lateral_area() const D_NOEXCEPT
    {
        return pi_of<value_type>() * m_radius * slant_height();
    }

    // closed_form_centroid
    //   on the cone axis, at h/4 above the base.
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type b_cart = System::to_cartesian(m_base);

        c_cart[0] = b_cart[0];
        c_cart[1] = b_cart[1];
        c_cart[2] = b_cart[2] +
                    m_height / static_cast<value_type>(4);

        return System::from_cartesian(c_cart);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t cone<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool cone<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool cone<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool cone<System>::is_3d;


//==============================================================================
// 7.  FRUSTUM (TRUNCATED CONE, +Z AXIS)
//==============================================================================

// frustum
//   struct: a right circular frustum (truncated cone) oriented along
// +z. Bottom disk has radius r1 at m_base; top disk has radius r2 at
// m_base + (0, 0, h). r1 == r2 reproduces a cylinder; r2 == 0
// reproduces a cone.
//
//   Volume        = (1/3) π h (r1² + r1 r2 + r2²)
//   Slant height  = √((r1 - r2)² + h²)
//   Surface area  = π (r1² + r2²) + π (r1 + r2) ℓ
template<typename System = cartesian<3, double> >
struct frustum
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "frustum: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_base;
    value_type m_radius_bottom;
    value_type m_radius_top;
    value_type m_height;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR frustum() D_NOEXCEPT
        : m_base(),
          m_radius_bottom(static_cast<value_type>(1)),
          m_radius_top(static_cast<value_type>(0.5)),
          m_height(static_cast<value_type>(1))
    {}

    D_CONSTEXPR frustum(
        const point_type& _base_centre,
        value_type        _r_bottom,
        value_type        _r_top,
        value_type        _h
    ) D_NOEXCEPT
        : m_base(_base_centre),
          m_radius_bottom(_r_bottom),
          m_radius_top(_r_top),
          m_height(_h)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   radius linearly interpolated from r1 at z=0 to r2 at z=h.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dz;
        value_type dx;
        value_type dy;
        value_type r_at_z;
        value_type rho_sq;
        value_type t;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type b_cart = System::to_cartesian(m_base);

        dz = p_cart[2] - b_cart[2];

        // outside height range
        if ( (dz < static_cast<value_type>(0)) ||
             (dz > m_height) )
        {
            return false;
        }

        // lerp the radius along the height
        t      = dz / m_height;
        r_at_z = m_radius_bottom +
                 t * (m_radius_top - m_radius_bottom);

        dx = p_cart[0] - b_cart[0];
        dy = p_cart[1] - b_cart[1];
        rho_sq = dx * dx + dy * dy;

        return (rho_sq <= (r_at_z * r_at_z));
    }

    // ---- closed-form measurements -------------------------------------------

    // slant_height
    //   √((r1 - r2)² + h²).
    value_type
    slant_height() const D_NOEXCEPT
    {
        value_type dr;

        dr = m_radius_bottom - m_radius_top;

        return std::sqrt(dr * dr + m_height * m_height);
    }

    // closed_form_volume
    //   (1/3) π h (r1² + r1 r2 + r2²).
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return pi_of<value_type>() *
               m_height *
               ( m_radius_bottom * m_radius_bottom +
                 m_radius_bottom * m_radius_top    +
                 m_radius_top    * m_radius_top ) /
               static_cast<value_type>(3);
    }

    // closed_form_surface_area
    //   π(r1² + r2²) + π(r1 + r2) ℓ.
    value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        value_type slant;

        slant = slant_height();

        return pi_of<value_type>() *
               ( m_radius_bottom * m_radius_bottom +
                 m_radius_top    * m_radius_top ) +
               pi_of<value_type>() *
               (m_radius_bottom + m_radius_top) *
               slant;
    }

    // lateral_area
    //   π(r1 + r2) ℓ.
    value_type
    lateral_area() const D_NOEXCEPT
    {
        return pi_of<value_type>() *
               (m_radius_bottom + m_radius_top) *
               slant_height();
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t frustum<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool frustum<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool frustum<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool frustum<System>::is_3d;


//==============================================================================
// 8.  TORUS
//==============================================================================

// torus
//   struct: a standard torus centred at m_center, with its symmetry
// axis along +z. Major radius R is the distance from the centre to
// the tube centre; minor radius r is the tube radius.
//
//   Volume        = 2 π² R r²
//   Surface area  = 4 π² R r
//   Centroid      = centre
//
// Requires R ≥ r for a non-self-intersecting torus; the type does
// not enforce this.
template<typename System = cartesian<3, double> >
struct torus
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "torus: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_major_radius;
    value_type m_minor_radius;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR torus() D_NOEXCEPT
        : m_center(),
          m_major_radius(static_cast<value_type>(2)),
          m_minor_radius(static_cast<value_type>(1))
    {}

    D_CONSTEXPR torus(
        const point_type& _centre,
        value_type        _major,
        value_type        _minor
    ) D_NOEXCEPT
        : m_center(_centre),
          m_major_radius(_major),
          m_minor_radius(_minor)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   the standard implicit torus inequality:
    //   (R - √(x² + y²))² + z² ≤ r²
    // evaluated in the centre-relative Cartesian frame.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dx;
        value_type dy;
        value_type dz;
        value_type rho;
        value_type radial_diff;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_center);

        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];
        dz = p_cart[2] - c_cart[2];

        rho         = std::sqrt(dx * dx + dy * dy);
        radial_diff = m_major_radius - rho;

        return ( (radial_diff * radial_diff + dz * dz) <=
                 (m_minor_radius * m_minor_radius) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   2 π² R r².
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() * pi_of<value_type>() *
               m_major_radius *
               m_minor_radius * m_minor_radius;
    }

    // closed_form_surface_area
    //   4 π² R r.
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(4) *
               pi_of<value_type>() * pi_of<value_type>() *
               m_major_radius *
               m_minor_radius;
    }

    // closed_form_centroid
    D_CONSTEXPR point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        return m_center;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t torus<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool torus<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool torus<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool torus<System>::is_3d;


//==============================================================================
// 9.  HEMISPHERE
//==============================================================================

// hemisphere
//   struct: the upper (+z) half of a sphere of radius r centred at
// m_center, including the equatorial disk. Surface area counts both
// the curved cap and the flat base.
//
//   Volume                = (2/3) π r³
//   Curved surface area   = 2 π r²
//   Total surface area    = 3 π r²        (2πr² cap + πr² disk)
//   Centroid (z-offset)   = 3r/8 above the centre
template<typename System = cartesian<3, double> >
struct hemisphere
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "hemisphere: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR hemisphere() D_NOEXCEPT
        : m_center(),
          m_radius(static_cast<value_type>(1))
    {}

    D_CONSTEXPR hemisphere(
        const point_type& _centre,
        value_type        _r
    ) D_NOEXCEPT
        : m_center(_centre),
          m_radius(_r)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   inside the sphere AND above the equator (z ≥ centre.z).
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dx;
        value_type dy;
        value_type dz;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_center);

        dz = p_cart[2] - c_cart[2];

        // below equator: not in the hemisphere
        if (dz < static_cast<value_type>(0))
        {
            return false;
        }

        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];

        return ( (dx * dx + dy * dy + dz * dz) <=
                 (m_radius * m_radius) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   (2/3) π r³.
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return ( static_cast<value_type>(2) / static_cast<value_type>(3) ) *
               pi_of<value_type>() *
               m_radius * m_radius * m_radius;
    }

    // closed_form_surface_area
    //   3 π r²  (curved cap 2πr² + flat disk πr²).
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(3) *
               pi_of<value_type>() *
               m_radius * m_radius;
    }

    // curved_area
    //   just the dome: 2πr².
    D_CONSTEXPR value_type
    curved_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() *
               m_radius * m_radius;
    }

    // closed_form_centroid
    //   on the axis, 3r/8 above the centre.
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type cc = System::to_cartesian(m_center);

        c_cart[0] = cc[0];
        c_cart[1] = cc[1];
        c_cart[2] = cc[2] +
                    static_cast<value_type>(3) * m_radius /
                    static_cast<value_type>(8);

        return System::from_cartesian(c_cart);
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t hemisphere<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool hemisphere<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool hemisphere<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool hemisphere<System>::is_3d;


//==============================================================================
// 10. SPHERICAL CAP
//==============================================================================

// spherical_cap
//   struct: the portion of a sphere of radius R cut off by a plane
// at height h above the equator. m_center is the sphere's centre;
// the cap consists of points {(x, y, z) : x²+y²+z² ≤ R² ∧ z ≥ R - h}
// in centre-relative coordinates. h ∈ [0, 2R]; h = R reproduces a
// hemisphere, h = 2R reproduces the whole sphere.
//
//   Volume                  = (π h² / 3) (3R - h)
//   Curved surface area     = 2 π R h
//   Base disk area          = π (2 R h - h²)
//   Total surface area      = 2 π R h + π (2 R h - h²)
template<typename System = cartesian<3, double> >
struct spherical_cap
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "spherical_cap: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_radius;
    value_type m_height;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR spherical_cap() D_NOEXCEPT
        : m_center(),
          m_radius(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(0.5))
    {}

    D_CONSTEXPR spherical_cap(
        const point_type& _sphere_centre,
        value_type        _r,
        value_type        _h
    ) D_NOEXCEPT
        : m_center(_sphere_centre),
          m_radius(_r),
          m_height(_h)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   inside the sphere AND z (centre-relative) ≥ R - h.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type dx;
        value_type dy;
        value_type dz;
        value_type z_cut;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type c_cart = System::to_cartesian(m_center);

        dz    = p_cart[2] - c_cart[2];
        z_cut = m_radius - m_height;

        // below the slicing plane: not in the cap
        if (dz < z_cut)
        {
            return false;
        }

        dx = p_cart[0] - c_cart[0];
        dy = p_cart[1] - c_cart[1];

        return ( (dx * dx + dy * dy + dz * dz) <=
                 (m_radius * m_radius) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   (π h² / 3) (3R - h).
    D_CONSTEXPR value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return pi_of<value_type>() *
               m_height * m_height *
               ( static_cast<value_type>(3) * m_radius - m_height ) /
               static_cast<value_type>(3);
    }

    // closed_form_surface_area
    //   curved (2πRh) + base disk (π(2Rh - h²)).
    D_CONSTEXPR value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() *
               m_radius * m_height +
               pi_of<value_type>() *
               ( static_cast<value_type>(2) * m_radius * m_height -
                 m_height * m_height );
    }

    // curved_area
    //   just the spherical surface: 2πRh.
    D_CONSTEXPR value_type
    curved_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) *
               pi_of<value_type>() *
               m_radius * m_height;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t spherical_cap<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR bool spherical_cap<System>::is_shape;
template<typename System>
D_CONSTEXPR_VAR bool spherical_cap<System>::is_2d;
template<typename System>
D_CONSTEXPR_VAR bool spherical_cap<System>::is_3d;


//==============================================================================
// 11. REGULAR PRISM
//==============================================================================

// regular_prism
//   struct: an N-sided regular polygon extruded along +z. Base is
// centred at m_base on the z = m_base.z plane; top face is at
// m_base + (0, 0, height). Circumradius is the distance from the
// axis to a base vertex; rotation gives the angular offset of vertex
// 0 from the +x direction.
//
//   Base area     = (1/2) N r² sin(2π/N)
//   Base perim    = 2 N r sin(π/N)
//   Volume        = base_area · height
//   Surface area  = 2 · base_area + base_perim · height
template<typename    System = cartesian<3, double>,
         std::size_t N      = 6>
struct regular_prism
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "regular_prism: 3D coordinate systems only.");
    D_STATIC_ASSERT((N >= 3),
                    "regular_prism: need at least 3 sides.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR std::size_t sides     = N;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_base;
    value_type m_circumradius;
    value_type m_height;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR regular_prism() D_NOEXCEPT
        : m_base(),
          m_circumradius(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {}

    D_CONSTEXPR regular_prism(
        const point_type& _base_centre,
        value_type        _r,
        value_type        _h,
        value_type        _rot = static_cast<value_type>(0)
    ) D_NOEXCEPT
        : m_base(_base_centre),
          m_circumradius(_r),
          m_height(_h),
          m_rotation(_rot)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   height test + point-in-(regular-polygon) test in xy. The
    // polygon test uses a 2D ray-cast over the implicit vertices.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type   dz;
        std::size_t  crossings;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type b_cart = System::to_cartesian(m_base);

        dz = p_cart[2] - b_cart[2];

        // outside the height range
        if ( (dz < static_cast<value_type>(0)) ||
             (dz > m_height) )
        {
            return false;
        }

        // 2D ray-cast against the base polygon vertices
        crossings = 0;

        for (std::size_t i = 0, j = (N - 1); i < N; j = i++)
        {
            value_type ang_i = m_rotation +
                two_pi_of<value_type>() *
                static_cast<value_type>(i) /
                static_cast<value_type>(N);
            value_type ang_j = m_rotation +
                two_pi_of<value_type>() *
                static_cast<value_type>(j) /
                static_cast<value_type>(N);

            value_type vi_x = b_cart[0] +
                m_circumradius * std::cos(ang_i);
            value_type vi_y = b_cart[1] +
                m_circumradius * std::sin(ang_i);
            value_type vj_x = b_cart[0] +
                m_circumradius * std::cos(ang_j);
            value_type vj_y = b_cart[1] +
                m_circumradius * std::sin(ang_j);

            // standard ray-cast crossing test
            if ( ((vi_y > p_cart[1]) != (vj_y > p_cart[1])) &&
                 ( p_cart[0] < ( (vj_x - vi_x) *
                                  (p_cart[1] - vi_y) /
                                  (vj_y - vi_y) +
                                  vi_x ) ) )
            {
                ++crossings;
            }
        }

        return ((crossings % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // base_area
    //   (1/2) N r² sin(2π/N).
    value_type
    base_area() const D_NOEXCEPT
    {
        value_type theta;

        theta = two_pi_of<value_type>() /
                static_cast<value_type>(N);

        return static_cast<value_type>(0.5) *
               static_cast<value_type>(N) *
               m_circumradius * m_circumradius *
               std::sin(theta);
    }

    // base_perimeter
    //   2 N r sin(π/N).
    value_type
    base_perimeter() const D_NOEXCEPT
    {
        value_type theta;

        theta = pi_of<value_type>() /
                static_cast<value_type>(N);

        return static_cast<value_type>(2) *
               static_cast<value_type>(N) *
               m_circumradius * std::sin(theta);
    }

    // closed_form_volume
    //   base_area * height.
    value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return base_area() * m_height;
    }

    // closed_form_surface_area
    //   2 * base_area + base_perimeter * height.
    value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return static_cast<value_type>(2) * base_area() +
               base_perimeter() * m_height;
    }

    // closed_form_centroid
    //   on the axis, half the height above the base.
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type b_cart = System::to_cartesian(m_base);

        c_cart[0] = b_cart[0];
        c_cart[1] = b_cart[1];
        c_cart[2] = b_cart[2] +
                    m_height / static_cast<value_type>(2);

        return System::from_cartesian(c_cart);
    }
};

template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t regular_prism<System, N>::dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t regular_prism<System, N>::sides;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_prism<System, N>::is_shape;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_prism<System, N>::is_2d;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_prism<System, N>::is_3d;


//==============================================================================
// 12. REGULAR PYRAMID
//==============================================================================

// regular_pyramid
//   struct: an N-sided regular polygon base with apex directly above
// the centre. Base is centred at m_base on z = m_base.z; apex is at
// m_base + (0, 0, height).
//
//   Base area      = (1/2) N r² sin(2π/N)
//   Side length    = 2 r sin(π/N)
//   Apothem        = r cos(π/N)
//   Slant height   = √(apothem² + height²)
//   Volume         = (1/3) base_area · height
//   Lateral area   = (1/2) base_perim · slant_height
//   Surface area   = base_area + lateral_area
template<typename    System = cartesian<3, double>,
         std::size_t N      = 4>
struct regular_pyramid
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "regular_pyramid: 3D coordinate systems only.");
    D_STATIC_ASSERT((N >= 3),
                    "regular_pyramid: need at least 3 sides.");

    // ---- type aliases -------------------------------------------------------

    typedef System coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    // ---- static constants ---------------------------------------------------

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR std::size_t sides     = N;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    point_type m_base;
    value_type m_circumradius;
    value_type m_height;
    value_type m_rotation;

    // ---- construction -------------------------------------------------------

    D_CONSTEXPR regular_pyramid() D_NOEXCEPT
        : m_base(),
          m_circumradius(static_cast<value_type>(1)),
          m_height(static_cast<value_type>(1)),
          m_rotation(static_cast<value_type>(0))
    {}

    D_CONSTEXPR regular_pyramid(
        const point_type& _base_centre,
        value_type        _r,
        value_type        _h,
        value_type        _rot = static_cast<value_type>(0)
    ) D_NOEXCEPT
        : m_base(_base_centre),
          m_circumradius(_r),
          m_height(_h),
          m_rotation(_rot)
    {}

    // ---- structural interface -----------------------------------------------

    // contains
    //   inside iff 0 ≤ dz ≤ h and the (x, y) projection lies inside
    // the scaled polygon r(1 - dz/h) at the current height.
    bool
    contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type   dz;
        value_type   r_at_z;
        std::size_t  crossings;

        const point_type p_cart = System::to_cartesian(_p);
        const point_type b_cart = System::to_cartesian(m_base);

        dz = p_cart[2] - b_cart[2];

        if ( (dz < static_cast<value_type>(0)) ||
             (dz > m_height) )
        {
            return false;
        }

        // polygon shrinks linearly to zero at the apex
        r_at_z = m_circumradius *
                 (static_cast<value_type>(1) - dz / m_height);

        // degenerate at the apex
        if (r_at_z <= static_cast<value_type>(0))
        {
            return false;
        }

        crossings = 0;

        // ray-cast against scaled-polygon vertices
        for (std::size_t i = 0, j = (N - 1); i < N; j = i++)
        {
            value_type ang_i = m_rotation +
                two_pi_of<value_type>() *
                static_cast<value_type>(i) /
                static_cast<value_type>(N);
            value_type ang_j = m_rotation +
                two_pi_of<value_type>() *
                static_cast<value_type>(j) /
                static_cast<value_type>(N);

            value_type vi_x = b_cart[0] +
                r_at_z * std::cos(ang_i);
            value_type vi_y = b_cart[1] +
                r_at_z * std::sin(ang_i);
            value_type vj_x = b_cart[0] +
                r_at_z * std::cos(ang_j);
            value_type vj_y = b_cart[1] +
                r_at_z * std::sin(ang_j);

            // standard ray-cast crossing test
            if ( ((vi_y > p_cart[1]) != (vj_y > p_cart[1])) &&
                 ( p_cart[0] < ( (vj_x - vi_x) *
                                  (p_cart[1] - vi_y) /
                                  (vj_y - vi_y) +
                                  vi_x ) ) )
            {
                ++crossings;
            }
        }

        return ((crossings % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // base_area
    value_type
    base_area() const D_NOEXCEPT
    {
        value_type theta;

        theta = two_pi_of<value_type>() /
                static_cast<value_type>(N);

        return static_cast<value_type>(0.5) *
               static_cast<value_type>(N) *
               m_circumradius * m_circumradius *
               std::sin(theta);
    }

    // base_perimeter
    value_type
    base_perimeter() const D_NOEXCEPT
    {
        value_type theta;

        theta = pi_of<value_type>() /
                static_cast<value_type>(N);

        return static_cast<value_type>(2) *
               static_cast<value_type>(N) *
               m_circumradius * std::sin(theta);
    }

    // apothem
    //   r cos(π/N): distance from centre to midpoint of a base side.
    value_type
    apothem() const D_NOEXCEPT
    {
        value_type theta;

        theta = pi_of<value_type>() /
                static_cast<value_type>(N);

        return m_circumradius * std::cos(theta);
    }

    // slant_height
    //   √(apothem² + height²).
    value_type
    slant_height() const D_NOEXCEPT
    {
        value_type a;

        a = apothem();

        return std::sqrt(a * a + m_height * m_height);
    }

    // closed_form_volume
    //   (1/3) base_area · height.
    value_type
    closed_form_volume() const D_NOEXCEPT
    {
        return base_area() * m_height /
               static_cast<value_type>(3);
    }

    // lateral_area
    //   (1/2) base_perimeter · slant_height.
    value_type
    lateral_area() const D_NOEXCEPT
    {
        return base_perimeter() * slant_height() /
               static_cast<value_type>(2);
    }

    // closed_form_surface_area
    //   base_area + lateral_area.
    value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        return base_area() + lateral_area();
    }

    // closed_form_centroid
    //   on the axis, h/4 above the base.
    point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type c_cart;

        const point_type b_cart = System::to_cartesian(m_base);

        c_cart[0] = b_cart[0];
        c_cart[1] = b_cart[1];
        c_cart[2] = b_cart[2] +
                    m_height / static_cast<value_type>(4);

        return System::from_cartesian(c_cart);
    }
};

template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t regular_pyramid<System, N>::dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t regular_pyramid<System, N>::sides;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_pyramid<System, N>::is_shape;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_pyramid<System, N>::is_2d;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool regular_pyramid<System, N>::is_3d;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_NAMED_3D_HPP
