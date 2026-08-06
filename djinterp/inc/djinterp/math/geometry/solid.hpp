/******************************************************************************
* djinterp [math]                                                solid.hpp
*
* General-purpose 3D shape composers.
*   Provides the four ways a user can describe an arbitrary 3D solid:
* by a triangle-mesh boundary, by a closed parametric surface
* (parametric_volume), by an implicit inequality (implicit_volume), or
* by an ordered tuple of heterogeneous surfaces (shape_from_surfaces).
* Named shapes (sphere, box, cylinder, etc.) live in named_3d.hpp.
*
* Each shape satisfies the structural shape interface from
* geometry_common.hpp; measurement (volume, surface area, centroid)
* lives in measure_3d.hpp.
*
* path:      /inc/djinterp/math/geometry/solid.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_SOLID_
#define DJINTERP_MATH_GEOMETRY_SOLID_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <tuple>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"
#include "./surface.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    INTERNAL RAY-TRIANGLE INTERSECTION
// ============================================================================

NS_INTERNAL

    // moller_trumbore
    //   helper: classic Möller-Trumbore ray-triangle intersection in
    // Cartesian space. Returns true and writes the hit parameter to
    // _t_out if the ray from _origin in direction _dir intersects
    // the triangle (_v0, _v1, _v2) at a forward (t > _epsilon)
    // distance. _dir need not be normalised.
    template<typename _T>
    bool
    moller_trumbore
    (
        const std::array<_T, 3>& _origin,
        const std::array<_T, 3>& _dir,
        const std::array<_T, 3>& _v0,
        const std::array<_T, 3>& _v1,
        const std::array<_T, 3>& _v2,
        _T&                      _t_out,
        _T                       _epsilon = static_cast<_T>(1e-9)
    ) noexcept
    {
        std::array<_T, 3> edge1;
        std::array<_T, 3> edge2;
        std::array<_T, 3> h;
        std::array<_T, 3> s;
        std::array<_T, 3> q;
        _T                a;
        _T                f;
        _T                u;
        _T                v;
        _T                t;

        edge1 = cartesian_sub_3<_T>(_v1, _v0);
        edge2 = cartesian_sub_3<_T>(_v2, _v0);

        // h = dir × edge2, a = edge1 · h
        h = cartesian_cross_3<_T>(_dir, edge2);
        a = cartesian_dot<_T, 3>(edge1, h);

        // parallel ray (a near zero) means no hit
        if ( (a > -_epsilon) && (a < _epsilon) )
        {
            return false;
        }

        f = static_cast<_T>(1) / a;
        s = cartesian_sub_3<_T>(_origin, _v0);
        u = f * cartesian_dot<_T, 3>(s, h);

        // first barycentric out of range
        if ( (u < static_cast<_T>(0)) ||
             (u > static_cast<_T>(1)) )
        {
            return false;
        }

        q = cartesian_cross_3<_T>(s, edge1);
        v = f * cartesian_dot<_T, 3>(_dir, q);

        // second barycentric out of range
        if ( (v < static_cast<_T>(0)) ||
             ((u + v) > static_cast<_T>(1)) )
        {
            return false;
        }

        t = f * cartesian_dot<_T, 3>(edge2, q);

        // require forward intersection
        if (t > _epsilon)
        {
            _t_out = t;
            return true;
        }

        return false;
    }

NS_END  // internal


// ============================================================================
// II.   TRIANGLE MESH
// ============================================================================

// triangle_mesh
//   struct: a polyhedron whose boundary is a list of _V vertices and
// _F triangular faces, each face being three indices into the vertex
// array. Closed-form surface area is the sum of triangle areas;
// closed-form volume uses the divergence theorem and is only valid
// for *closed manifold* meshes with consistent outward-pointing
// face orientation:
//
//   V = (1/6) Σ_face (v0 · (v1 × v2))
//
// Containment is by ray-casting against all faces.
template<typename    _System,
         std::size_t _V,
         std::size_t _F>
struct triangle_mesh
{
    static_assert(_System::dimension == 3,
                  "triangle_mesh: 3D coordinate systems only.");
    static_assert(_V >= 4,
                  "triangle_mesh: need at least 4 vertices to enclose "
                  "a volume (tetrahedron is minimum).");
    static_assert(_F >= 4,
                  "triangle_mesh: need at least 4 faces to enclose a "
                  "volume.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;
    using face_type         = std::array<std::size_t, 3>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension    = 3;
    static constexpr std::size_t vertex_count = _V;
    static constexpr std::size_t face_count   = _F;
    static constexpr bool        is_shape     = true;
    static constexpr bool        is_2d        = false;
    static constexpr bool        is_3d        = true;

    // ---- data ---------------------------------------------------------------

    std::array<point_type, _V> m_vertices;
    std::array<face_type,  _F> m_faces;

    // ---- construction -------------------------------------------------------

    constexpr triangle_mesh() noexcept
        : m_vertices{},
          m_faces{}
    {
    }

    constexpr triangle_mesh(
        const std::array<point_type, _V>& _verts,
        const std::array<face_type,  _F>& _faces
    ) noexcept
        : m_vertices(_verts),
          m_faces(_faces)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   ray-casting against the triangle mesh in Cartesian. The ray
    // shoots along +x; an odd intersection count means inside. For
    // points that lie exactly on a vertex or edge of a face the
    // result is undefined - a small jitter on the ray direction can
    // be added by the caller if robustness is needed.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t            hits;
        std::array<value_type, 3> dir;
        value_type             t;

        auto p_cart = _System::to_cartesian(_p);
        hits        = 0;

        // ray along +x (arbitrary choice)
        dir[0] = static_cast<value_type>(1);
        dir[1] = static_cast<value_type>(0);
        dir[2] = static_cast<value_type>(0);

        // count forward ray hits against every face
        for (std::size_t i = 0; i < _F; ++i)
        {
            auto v0 = _System::to_cartesian(
                m_vertices[m_faces[i][0]]);
            auto v1 = _System::to_cartesian(
                m_vertices[m_faces[i][1]]);
            auto v2 = _System::to_cartesian(
                m_vertices[m_faces[i][2]]);

            if (internal::moller_trumbore<value_type>(
                    p_cart, dir, v0, v1, v2, t))
            {
                ++hits;
            }
        }

        return ((hits % 2) == 1);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_surface_area
    //   sum of |edge1 × edge2| / 2 over all faces.
    value_type
    closed_form_surface_area
    () const noexcept
    {
        value_type total;

        total = static_cast<value_type>(0);

        // accumulate area per face
        for (std::size_t i = 0; i < _F; ++i)
        {
            auto v0 = _System::to_cartesian(
                m_vertices[m_faces[i][0]]);
            auto v1 = _System::to_cartesian(
                m_vertices[m_faces[i][1]]);
            auto v2 = _System::to_cartesian(
                m_vertices[m_faces[i][2]]);

            auto e1 = cartesian_sub_3<value_type>(v1, v0);
            auto e2 = cartesian_sub_3<value_type>(v2, v0);
            auto cr = cartesian_cross_3<value_type>(e1, e2);

            value_type mag = std::sqrt(cr[0] * cr[0] +
                                       cr[1] * cr[1] +
                                       cr[2] * cr[2]);

            total += mag / static_cast<value_type>(2);
        }

        return total;
    }

    // closed_form_volume
    //   divergence-theorem volume of a closed manifold mesh:
    //   V = (1/6) | Σ_face (v0 · (v1 × v2)) |.
    // Only meaningful when faces are consistently oriented outward
    // and the mesh is closed; the absolute value protects against
    // inverted winding but cannot fix open meshes.
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type sum;
        value_type result;

        sum = static_cast<value_type>(0);

        // accumulate signed tetrahedron volumes
        for (std::size_t i = 0; i < _F; ++i)
        {
            auto v0 = _System::to_cartesian(
                m_vertices[m_faces[i][0]]);
            auto v1 = _System::to_cartesian(
                m_vertices[m_faces[i][1]]);
            auto v2 = _System::to_cartesian(
                m_vertices[m_faces[i][2]]);

            auto cr = cartesian_cross_3<value_type>(v1, v2);

            sum += cartesian_dot<value_type, 3>(v0, cr);
        }

        result = sum / static_cast<value_type>(6);

        // absolute value (orientation independent)
        if (result < static_cast<value_type>(0))
        {
            result = -result;
        }

        return result;
    }

    // closed_form_centroid
    //   volume-weighted centroid via signed tetrahedral
    // decomposition. Like closed_form_volume, this assumes a closed,
    // consistently-oriented manifold mesh.
    point_type
    closed_form_centroid
    () const noexcept
    {
        std::array<value_type, 3> cx_acc;
        value_type                volume_sum;
        std::array<value_type, 3> centre;

        cx_acc[0]  = static_cast<value_type>(0);
        cx_acc[1]  = static_cast<value_type>(0);
        cx_acc[2]  = static_cast<value_type>(0);
        volume_sum = static_cast<value_type>(0);

        // tetrahedra from origin to each face
        for (std::size_t i = 0; i < _F; ++i)
        {
            auto v0 = _System::to_cartesian(
                m_vertices[m_faces[i][0]]);
            auto v1 = _System::to_cartesian(
                m_vertices[m_faces[i][1]]);
            auto v2 = _System::to_cartesian(
                m_vertices[m_faces[i][2]]);

            auto cr   = cartesian_cross_3<value_type>(v1, v2);
            value_type vol6 = cartesian_dot<value_type, 3>(v0, cr);

            // tetrahedron centroid is (v0 + v1 + v2) / 4 - origin is
            // implicit at 0, contributing the remaining 1/4 weight
            cx_acc[0] += vol6 * (v0[0] + v1[0] + v2[0]) /
                         static_cast<value_type>(4);
            cx_acc[1] += vol6 * (v0[1] + v1[1] + v2[1]) /
                         static_cast<value_type>(4);
            cx_acc[2] += vol6 * (v0[2] + v1[2] + v2[2]) /
                         static_cast<value_type>(4);

            volume_sum += vol6;
        }

        // guard against degenerate mesh
        if (volume_sum == static_cast<value_type>(0))
        {
            return m_vertices[0];
        }

        centre[0] = cx_acc[0] / volume_sum;
        centre[1] = cx_acc[1] / volume_sum;
        centre[2] = cx_acc[2] / volume_sum;

        return _System::from_cartesian(centre);
    }
};


// ============================================================================
// III.  PARAMETRIC VOLUME
// ============================================================================

// parametric_volume
//   struct: a 3D solid bounded by a single closed parametric
// surface. The user supplies a surface whose at(u, v) traces a
// closed manifold (typically the boundary of the desired solid);
// closure is assumed, not enforced.
//
// Volume and surface area are computed numerically by measure_3d;
// no closed_form_* members are provided.
template<typename _Surface>
struct parametric_volume
{
    static_assert(is_surface<_Surface>::value,
                  "parametric_volume: _Surface must satisfy the "
                  "surface interface.");
    static_assert(_Surface::dimension == 3,
                  "parametric_volume: 3D surfaces only.");

    // ---- type aliases -------------------------------------------------------

    using boundary_type     = _Surface;
    using coordinate_system = typename _Surface::coordinate_system;
    using value_type        = typename _Surface::value_type;
    using point_type        = typename _Surface::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 3;
    static constexpr bool        is_shape  = true;
    static constexpr bool        is_2d     = false;
    static constexpr bool        is_3d     = true;

    // ---- data ---------------------------------------------------------------

    boundary_type m_boundary;

    // ---- construction -------------------------------------------------------

    constexpr parametric_volume() noexcept
        : m_boundary{}
    {
    }

    constexpr explicit parametric_volume(
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
    //   sampled ray-casting: tessellate the parametric surface into
    // a _Samples x _Samples grid of quads (two triangles each) and
    // count ray hits along +x. _Samples controls accuracy versus
    // cost.
    template<std::size_t _Samples = 32>
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t                hits;
        std::array<value_type, 3>  dir;
        value_type                 u_min;
        value_type                 u_max;
        value_type                 v_min;
        value_type                 v_max;
        value_type                 du;
        value_type                 dv;
        value_type                 t;

        auto p_cart = coordinate_system::to_cartesian(_p);
        hits        = 0;

        // ray along +x
        dir[0] = static_cast<value_type>(1);
        dir[1] = static_cast<value_type>(0);
        dir[2] = static_cast<value_type>(0);

        u_min = m_boundary.parameter_min_u();
        u_max = m_boundary.parameter_max_u();
        v_min = m_boundary.parameter_min_v();
        v_max = m_boundary.parameter_max_v();
        du    = (u_max - u_min) /
                static_cast<value_type>(_Samples);
        dv    = (v_max - v_min) /
                static_cast<value_type>(_Samples);

        // walk grid cells, ray-cast against two triangles per cell
        for (std::size_t i = 0; i < _Samples; ++i)
        {
            for (std::size_t j = 0; j < _Samples; ++j)
            {
                value_type u0 = u_min + static_cast<value_type>(i) * du;
                value_type u1 = u0 + du;
                value_type v0 = v_min + static_cast<value_type>(j) * dv;
                value_type v1 = v0 + dv;

                auto p00 = coordinate_system::to_cartesian(
                    m_boundary.at(u0, v0));
                auto p10 = coordinate_system::to_cartesian(
                    m_boundary.at(u1, v0));
                auto p01 = coordinate_system::to_cartesian(
                    m_boundary.at(u0, v1));
                auto p11 = coordinate_system::to_cartesian(
                    m_boundary.at(u1, v1));

                // triangle 1: p00, p10, p11
                if (internal::moller_trumbore<value_type>(
                        p_cart, dir, p00, p10, p11, t))
                {
                    ++hits;
                }

                // triangle 2: p00, p11, p01
                if (internal::moller_trumbore<value_type>(
                        p_cart, dir, p00, p11, p01, t))
                {
                    ++hits;
                }
            }
        }

        return ((hits % 2) == 1);
    }
};


// ============================================================================
// IV.   IMPLICIT VOLUME
// ============================================================================

// implicit_volume
//   struct: a 3D solid defined by F(x, y, z) ≤ 0. Wraps an
// expression from the math framework that satisfies the standard
// evaluate() interface and depends on three variables.
//
// No closed-form measurements are provided; numerical surface area
// and volume require Monte Carlo or grid sampling, which the user
// can drive externally with the predicates here.
template<typename _Expr,
         typename _System = cartesian<3, double>>
struct implicit_volume
{
    static_assert(_System::dimension == 3,
                  "implicit_volume: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using expression_type   = _Expr;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension   = 3;
    static constexpr bool        is_shape    = true;
    static constexpr bool        is_2d       = false;
    static constexpr bool        is_3d       = true;
    static constexpr bool        is_implicit = true;

    // ---- structural interface -----------------------------------------------

    // contains
    //   true when F(x, y, z) ≤ 0.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto cart  = _System::to_cartesian(_p);
        auto input = std::make_tuple(cart[0], cart[1], cart[2]);

        return ( _Expr::evaluate(input) <=
                 static_cast<value_type>(0) );
    }

    // on_boundary
    //   true when |F(x, y, z)| < _epsilon.
    bool
    on_boundary
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


// ============================================================================
// V.    SHAPE FROM SURFACES (heterogeneous boundary)
// ============================================================================

// shape_from_surfaces
//   struct: a 3D solid whose boundary is an ordered tuple of
// heterogeneous surfaces. The user is responsible for ensuring the
// surfaces fit together along consistent edges and that face
// normals consistently point outward.
//
// All surfaces must share the same coordinate system, taken from
// the first surface.
template<typename... _Surfaces>
struct shape_from_surfaces
{
    static_assert(sizeof...(_Surfaces) >= 1,
                  "shape_from_surfaces: at least one surface required.");

    // ---- type aliases -------------------------------------------------------

    using first_surface_type = std::tuple_element_t<
        0, std::tuple<_Surfaces...>>;
    using coordinate_system  =
        typename first_surface_type::coordinate_system;
    using value_type         =
        typename coordinate_system::value_type;
    using point_type         =
        typename coordinate_system::point_type;
    using surfaces_type      = std::tuple<_Surfaces...>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      =
        coordinate_system::dimension;
    static constexpr std::size_t surface_count  = sizeof...(_Surfaces);
    static constexpr bool        is_shape       = true;
    static constexpr bool        is_2d          = false;
    static constexpr bool        is_3d          = true;

    // ---- data ---------------------------------------------------------------

    surfaces_type m_surfaces;

    // ---- construction -------------------------------------------------------

    constexpr shape_from_surfaces() noexcept
        : m_surfaces{}
    {
    }

    constexpr explicit shape_from_surfaces(
        const _Surfaces&... _s
    ) noexcept
        : m_surfaces(_s...)
    {
    }

    // ---- access -------------------------------------------------------------

    const surfaces_type&
    surfaces
    () const noexcept
    {
        return m_surfaces;
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   sampled ray-casting against each surface's tessellation.
    // _SamplesPerSurface controls grid resolution per surface.
    template<std::size_t _SamplesPerSurface = 16>
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        std::size_t                hits;
        std::array<value_type, 3>  dir;

        auto p_cart = coordinate_system::to_cartesian(_p);
        hits        = 0;

        dir[0] = static_cast<value_type>(1);
        dir[1] = static_cast<value_type>(0);
        dir[2] = static_cast<value_type>(0);

        // visit each surface in the tuple
        std::apply(
            [&](const auto&... _surf)
            {
                ( count_surface_hits<_SamplesPerSurface>(
                      _surf, p_cart, dir, hits),
                  ... );
            },
            m_surfaces);

        return ((hits % 2) == 1);
    }

private:
    // count_surface_hits
    //   helper: tessellate one surface into a grid of triangle pairs
    // and accumulate forward ray hits against them.
    template<std::size_t _SPS,
             typename    _SingleSurface>
    static void
    count_surface_hits
    (
        const _SingleSurface&            _surf,
        const std::array<value_type, 3>& _origin,
        const std::array<value_type, 3>& _dir,
        std::size_t&                     _hits
    ) noexcept
    {
        value_type u_min;
        value_type u_max;
        value_type v_min;
        value_type v_max;
        value_type du;
        value_type dv;
        value_type t;

        u_min = _surf.parameter_min_u();
        u_max = _surf.parameter_max_u();
        v_min = _surf.parameter_min_v();
        v_max = _surf.parameter_max_v();
        du    = (u_max - u_min) /
                static_cast<value_type>(_SPS);
        dv    = (v_max - v_min) /
                static_cast<value_type>(_SPS);

        // grid of quads, two triangles each
        for (std::size_t i = 0; i < _SPS; ++i)
        {
            for (std::size_t j = 0; j < _SPS; ++j)
            {
                value_type u0 = u_min +
                    static_cast<value_type>(i) * du;
                value_type u1 = u0 + du;
                value_type v0 = v_min +
                    static_cast<value_type>(j) * dv;
                value_type v1 = v0 + dv;

                auto p00 = coordinate_system::to_cartesian(
                    _surf.at(u0, v0));
                auto p10 = coordinate_system::to_cartesian(
                    _surf.at(u1, v0));
                auto p01 = coordinate_system::to_cartesian(
                    _surf.at(u0, v1));
                auto p11 = coordinate_system::to_cartesian(
                    _surf.at(u1, v1));

                if (internal::moller_trumbore<value_type>(
                        _origin, _dir, p00, p10, p11, t))
                {
                    ++_hits;
                }

                if (internal::moller_trumbore<value_type>(
                        _origin, _dir, p00, p11, p01, t))
                {
                    ++_hits;
                }
            }
        }

        return;
    }
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SOLID_
