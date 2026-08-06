/******************************************************************************
* djinterp [math]                                                  surface.hpp
*
* 2-manifold surface primitives embedded in 3D (and higher) space.
*   The structural surface interface mirrors the edge interface but
* with two parameters (u, v) instead of one; surfaces serve as the
* 3D analogue of edges and as the boundary pieces for 3D shapes.
* Conversions between coordinate systems pivot through Cartesian, so a
* triangle_surface anchored by spherical points still traces the
* triangle the user expects in physical space.
*
* STRUCTURAL SURFACE INTERFACE (compile-time detectable):
*   - using coordinate_system, value_type, point_type
*   - static constexpr std::size_t dimension                 (embedding)
*   - static constexpr std::size_t parametric_dimension = 2
*   - static constexpr bool is_surface = true
*   - static constexpr bool is_curved
*   - static constexpr bool is_closed
*   - value_type parameter_min_u() const, parameter_max_u() const
*   - value_type parameter_min_v() const, parameter_max_v() const
*   - point_type at(value_type _u, value_type _v) const
*   - (optional) value_type closed_form_area() const
*
* PROVIDED TYPES:
*   triangle_surface<System>             - 3 vertices, planar patch
*   planar_polygon_surface<System, N>    - N coplanar vertices
*   bilinear_patch<System>               - 4 corners, ruled surface
*   parametric_surface_edge<Surf, System>- wraps math::parametric_surface
*   implicit_surface<Expr, System>       - zero level set F(x,y,z) = 0
*
* path:      /inc/djinterp/math/geometry/surface.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_SURFACE_
#define DJINTERP_MATH_GEOMETRY_SURFACE_ 1

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
// I.    TRIANGLE SURFACE
// ============================================================================

// triangle_surface
//   struct: a planar triangular patch in 3D (or higher) space
// defined by three vertices. Parameter convention: barycentric, with
// at(u, v) = (1 - u - v) v0 + u v1 + v v2. The valid sub-domain is
// the simplex u ≥ 0, v ≥ 0, u + v ≤ 1; outside that region, at(u, v)
// extrapolates linearly, which is occasionally useful and never
// surprising for users familiar with barycentric coords.
template<typename _System = cartesian<3, double>>
struct triangle_surface
{
    static_assert(_System::dimension >= 3,
                  "triangle_surface: embedding dimension must be at "
                  "least 3 (use math::triangle for 2D).");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension            = _System::dimension;
    static constexpr std::size_t parametric_dimension = 2;
    static constexpr bool        is_surface           = true;
    static constexpr bool        is_curved            = false;
    static constexpr bool        is_closed            = false;

    // ---- data ---------------------------------------------------------------

    point_type m_v0;
    point_type m_v1;
    point_type m_v2;

    // ---- construction -------------------------------------------------------

    constexpr triangle_surface() noexcept
        : m_v0{},
          m_v1{},
          m_v2{}
    {
    }

    constexpr triangle_surface(
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

    constexpr value_type
    parameter_min_u
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_u
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    constexpr value_type
    parameter_min_v
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_v
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // at
    //   evaluates the barycentric blend at (_u, _v) in Cartesian space
    // and maps back into the user's coordinate system.
    point_type
    at
    (
        value_type _u,
        value_type _v
    ) const noexcept
    {
        value_type                          w;
        std::array<value_type, dimension>   p;

        // third barycentric coord (no clamping; extrapolation allowed)
        w = static_cast<value_type>(1) - _u - _v;

        auto c0 = _System::to_cartesian(m_v0);
        auto c1 = _System::to_cartesian(m_v1);
        auto c2 = _System::to_cartesian(m_v2);

        // barycentric blend
        for (std::size_t i = 0; i < dimension; ++i)
        {
            p[i] = w * c0[i] + _u * c1[i] + _v * c2[i];
        }

        return _System::from_cartesian(p);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   |cross(v1 - v0, v2 - v0)| / 2. Only meaningful in 3D
    // embedding; for higher dimensions, fall back to numerical
    // measurement (the cross product specialises to 3D).
    template<std::size_t _D = dimension,
             typename = std::enable_if_t<(_D == 3)>>
    value_type
    closed_form_area
    () const noexcept
    {
        value_type magnitude_sq;

        auto c0 = _System::to_cartesian(m_v0);
        auto c1 = _System::to_cartesian(m_v1);
        auto c2 = _System::to_cartesian(m_v2);

        auto e1 = cartesian_sub_3<value_type>(c1, c0);
        auto e2 = cartesian_sub_3<value_type>(c2, c0);
        auto cr = cartesian_cross_3<value_type>(e1, e2);

        magnitude_sq = cr[0] * cr[0] + cr[1] * cr[1] + cr[2] * cr[2];

        return std::sqrt(magnitude_sq) /
               static_cast<value_type>(2);
    }

    // normal
    //   returns the unit normal (in Cartesian) of the triangle's
    // plane. Direction follows the right-hand rule on (v0, v1, v2).
    template<std::size_t _D = dimension,
             typename = std::enable_if_t<(_D == 3)>>
    std::array<value_type, 3>
    normal
    () const noexcept
    {
        value_type len;

        auto c0 = _System::to_cartesian(m_v0);
        auto c1 = _System::to_cartesian(m_v1);
        auto c2 = _System::to_cartesian(m_v2);

        auto e1 = cartesian_sub_3<value_type>(c1, c0);
        auto e2 = cartesian_sub_3<value_type>(c2, c0);
        auto n  = cartesian_cross_3<value_type>(e1, e2);

        len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);

        // degenerate triangle: return zero vector
        if (len == static_cast<value_type>(0))
        {
            return {{ static_cast<value_type>(0),
                      static_cast<value_type>(0),
                      static_cast<value_type>(0) }};
        }

        return {{ n[0] / len, n[1] / len, n[2] / len }};
    }
};


// ============================================================================
// II.   PLANAR POLYGON SURFACE
// ============================================================================

// planar_polygon_surface
//   struct: an N-vertex polygon embedded in 3D space. The vertices
// are assumed (not verified) to be coplanar; the user is responsible
// for that invariant. Closed-form area is the sum of triangle areas
// from the first vertex, equivalent to a 3D shoelace formula.
//
// at(u, v) is not provided - parameterising an N-gon over [0,1]² has
// no canonical embedding. Users who need parametric sampling should
// triangulate the polygon and use triangle_surface for each piece.
template<typename    _System,
         std::size_t _N>
struct planar_polygon_surface
{
    static_assert(_System::dimension >= 3,
                  "planar_polygon_surface: embedding dimension must be "
                  "at least 3 (use math::polygon for 2D).");
    static_assert(_N >= 3,
                  "planar_polygon_surface: at least 3 vertices required.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension            = _System::dimension;
    static constexpr std::size_t parametric_dimension = 2;
    static constexpr std::size_t vertex_count         = _N;
    static constexpr bool        is_surface           = true;
    static constexpr bool        is_curved            = false;
    static constexpr bool        is_closed            = false;

    // ---- data ---------------------------------------------------------------

    std::array<point_type, _N> m_vertices;

    // ---- construction -------------------------------------------------------

    constexpr planar_polygon_surface() noexcept
        : m_vertices{}
    {
    }

    constexpr explicit planar_polygon_surface(
        const std::array<point_type, _N>& _verts
    ) noexcept
        : m_vertices(_verts)
    {
    }

    // ---- structural interface (degenerate) ----------------------------------

    // parameter_min_u / parameter_max_u / parameter_min_v / parameter_max_v
    //   we expose [0, 1]² for type-check compatibility, but at(u, v)
    // is intentionally absent: see header comment.
    constexpr value_type
    parameter_min_u
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_u
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    constexpr value_type
    parameter_min_v
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_v
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_area
    //   sum of triangle areas using the first vertex as the fan apex.
    template<std::size_t _D = dimension,
             typename = std::enable_if_t<(_D == 3)>>
    value_type
    closed_form_area
    () const noexcept
    {
        std::array<value_type, 3> n_total;
        value_type                magnitude_sq;

        n_total[0] = static_cast<value_type>(0);
        n_total[1] = static_cast<value_type>(0);
        n_total[2] = static_cast<value_type>(0);

        auto c0 = _System::to_cartesian(m_vertices[0]);

        // accumulate the vector area of each fan triangle
        for (std::size_t i = 1; (i + 1) < _N; ++i)
        {
            auto c1 = _System::to_cartesian(m_vertices[i]);
            auto c2 = _System::to_cartesian(m_vertices[i + 1]);

            auto e1 = cartesian_sub_3<value_type>(c1, c0);
            auto e2 = cartesian_sub_3<value_type>(c2, c0);
            auto cr = cartesian_cross_3<value_type>(e1, e2);

            n_total[0] += cr[0];
            n_total[1] += cr[1];
            n_total[2] += cr[2];
        }

        // half magnitude gives the polygon area
        magnitude_sq = n_total[0] * n_total[0] +
                       n_total[1] * n_total[1] +
                       n_total[2] * n_total[2];

        return std::sqrt(magnitude_sq) /
               static_cast<value_type>(2);
    }
};


// ============================================================================
// III.  BILINEAR PATCH
// ============================================================================

// bilinear_patch
//   struct: a ruled surface defined by four corner points P00, P10,
// P01, P11, with parameter convention
//   at(u, v) = (1-u)(1-v) P00 + u(1-v) P10 + (1-u)v P01 + uv P11.
// Generally non-planar; area has no clean closed form (involves
// elliptic integrals), so users should call surface_area() in
// measure_3d.hpp to integrate numerically.
template<typename _System = cartesian<3, double>>
struct bilinear_patch
{
    static_assert(_System::dimension >= 3,
                  "bilinear_patch: embedding dimension must be at least 3.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension            = _System::dimension;
    static constexpr std::size_t parametric_dimension = 2;
    static constexpr bool        is_surface           = true;
    static constexpr bool        is_curved            = true;
    static constexpr bool        is_closed            = false;

    // ---- data ---------------------------------------------------------------

    point_type m_p00;
    point_type m_p10;
    point_type m_p01;
    point_type m_p11;

    // ---- construction -------------------------------------------------------

    constexpr bilinear_patch() noexcept
        : m_p00{},
          m_p10{},
          m_p01{},
          m_p11{}
    {
    }

    constexpr bilinear_patch(
        const point_type& _corner_00,
        const point_type& _corner_10,
        const point_type& _corner_01,
        const point_type& _corner_11
    ) noexcept
        : m_p00(_corner_00),
          m_p10(_corner_10),
          m_p01(_corner_01),
          m_p11(_corner_11)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min_u
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_u
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    constexpr value_type
    parameter_min_v
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_v
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // at
    //   bilinear blend in Cartesian space.
    point_type
    at
    (
        value_type _u,
        value_type _v
    ) const noexcept
    {
        value_type                          uu;
        value_type                          vv;
        value_type                          w00;
        value_type                          w10;
        value_type                          w01;
        value_type                          w11;
        std::array<value_type, dimension>   p;

        uu = static_cast<value_type>(1) - _u;
        vv = static_cast<value_type>(1) - _v;

        w00 = uu * vv;
        w10 = _u * vv;
        w01 = uu * _v;
        w11 = _u * _v;

        auto c00 = _System::to_cartesian(m_p00);
        auto c10 = _System::to_cartesian(m_p10);
        auto c01 = _System::to_cartesian(m_p01);
        auto c11 = _System::to_cartesian(m_p11);

        // weighted sum of the four corners
        for (std::size_t i = 0; i < dimension; ++i)
        {
            p[i] = w00 * c00[i] +
                   w10 * c10[i] +
                   w01 * c01[i] +
                   w11 * c11[i];
        }

        return _System::from_cartesian(p);
    }
};


// ============================================================================
// IV.   PARAMETRIC SURFACE EDGE (wraps math::parametric_surface)
// ============================================================================

// parametric_surface_edge
//   struct: a bounded portion of a math::parametric_surface. The
// wrapped surface's evaluate(u, v) produces Cartesian components;
// this type stores the (u, v) bounding rectangle as runtime members
// and maps each sampled point into the user's coordinate system.
//
// Usage:
//   using sphere_surf = parametric_surface<
//       sin_fn<var_t<>> * cos_fn<var_2_t<>>,    // x(θ,φ)
//       sin_fn<var_t<>> * sin_fn<var_2_t<>>,    // y(θ,φ)
//       cos_fn<var_t<>>                         // z(θ)
//   >;
//   parametric_surface_edge<sphere_surf> sphere_patch{
//       0.0, pi_v<double>, 0.0, two_pi_v<double>};
template<typename _ParametricSurface,
         typename _System = cartesian<
             _ParametricSurface::output_dimension, double>>
struct parametric_surface_edge
{
    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using parametric_type   = _ParametricSurface;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension            = _System::dimension;
    static constexpr std::size_t parametric_dimension = 2;
    static constexpr bool        is_surface           = true;
    static constexpr bool        is_curved            = true;
    static constexpr bool        is_closed            = false;

    static_assert(_ParametricSurface::output_dimension == _System::dimension,
                  "parametric_surface_edge: surface output dimension must "
                  "match coordinate-system dimension.");

    // ---- data ---------------------------------------------------------------

    value_type m_u_min;
    value_type m_u_max;
    value_type m_v_min;
    value_type m_v_max;

    // ---- construction -------------------------------------------------------

    constexpr parametric_surface_edge() noexcept
        : m_u_min(static_cast<value_type>(0)),
          m_u_max(static_cast<value_type>(1)),
          m_v_min(static_cast<value_type>(0)),
          m_v_max(static_cast<value_type>(1))
    {
    }

    constexpr parametric_surface_edge(
        value_type _u_lower,
        value_type _u_upper,
        value_type _v_lower,
        value_type _v_upper
    ) noexcept
        : m_u_min(_u_lower),
          m_u_max(_u_upper),
          m_v_min(_v_lower),
          m_v_max(_v_upper)
    {
    }

    // ---- structural interface -----------------------------------------------

    constexpr value_type
    parameter_min_u
    () const noexcept
    {
        return m_u_min;
    }

    constexpr value_type
    parameter_max_u
    () const noexcept
    {
        return m_u_max;
    }

    constexpr value_type
    parameter_min_v
    () const noexcept
    {
        return m_v_min;
    }

    constexpr value_type
    parameter_max_v
    () const noexcept
    {
        return m_v_max;
    }

    // at
    //   evaluates the wrapped parametric_surface at (_u, _v) and maps
    // the Cartesian result into the user's system.
    point_type
    at
    (
        value_type _u,
        value_type _v
    ) const noexcept
    {
        std::array<value_type, dimension> cart{};

        // math::parametric_surface returns std::array in cartesian
        auto raw = _ParametricSurface::evaluate(_u, _v);

        for (std::size_t i = 0; i < dimension; ++i)
        {
            cart[i] = static_cast<value_type>(raw[i]);
        }

        if constexpr (_System::is_cartesian)
        {
            return cart;
        }
        else
        {
            return _System::from_cartesian(cart);
        }
    }
};


// ============================================================================
// V.    IMPLICIT SURFACE
// ============================================================================

// implicit_surface
//   struct: a 2-manifold defined by F(x, y, z) = 0. Like
// implicit_edge in 2D, this type has no parametric at(u, v); it
// exists primarily as a containment predicate (on_surface) and as a
// boundary type for implicit_volume.
template<typename _Expr,
         typename _System = cartesian<3, double>>
struct implicit_surface
{
    static_assert(_System::dimension == 3,
                  "implicit_surface: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using expression_type   = _Expr;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension            = 3;
    static constexpr std::size_t parametric_dimension = 2;
    static constexpr bool        is_surface           = true;
    static constexpr bool        is_curved            = true;
    static constexpr bool        is_closed            = false;
    static constexpr bool        is_implicit          = true;

    // ---- structural interface (degenerate) ----------------------------------

    constexpr value_type
    parameter_min_u
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_u
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    constexpr value_type
    parameter_min_v
    () const noexcept
    {
        return static_cast<value_type>(0);
    }

    constexpr value_type
    parameter_max_v
    () const noexcept
    {
        return static_cast<value_type>(1);
    }

    // ---- queries ------------------------------------------------------------

    // on_surface
    //   true when |F(x, y, z)| < _epsilon, i.e. the point is on the
    // implicit surface up to numerical tolerance.
    bool
    on_surface
    (
        const point_type& _p,
        value_type        _epsilon = static_cast<value_type>(1e-9)
    ) const noexcept
    {
        value_type v;

        auto cart  = _System::to_cartesian(_p);
        auto input = std::make_tuple(cart[0], cart[1], cart[2]);

        v = _Expr::evaluate(input);

        if (v < static_cast<value_type>(0))
        {
            v = -v;
        }

        return (v < _epsilon);
    }
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SURFACE_
