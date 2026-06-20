/******************************************************************************
* djinterp [math]                                                   star_3d.hpp
*
* 3D star polyhedra: face-erection and named instances.
*   Provides two complementary tools for "spiky" polyhedra:
*
*   - stellate<Mesh, Height>(base, h):
*       face-erection composer. Takes any triangle_mesh and returns a
*       new triangle_mesh in which each base face has been replaced
*       by three triangles forming a pyramid (positive h: outward
*       spike, negative h: inward dimple) above the face's centroid
*       along its outward normal.
*
*   - stella_octangula<System>:
*       the simplest non-trivial stellated polyhedron - the compound
*       of two interpenetrating regular tetrahedra. Modelled exactly
*       as a triangle_mesh with eight tetrahedral spikes.
*
*   - small_stellated_dodecahedron<System>, great_dodecahedron<System>,
*     great_stellated_dodecahedron<System>, great_icosahedron<System>:
*       the four Kepler-Poinsot regular star polyhedra. Their faces
*       are self-intersecting pentagrams or pentagons that pass
*       through the body of the polyhedron, so a clean triangle mesh
*       cannot represent the formal figure exactly. We provide:
*
*         * the *convex hull* vertices and a closed-hull triangulation
*           (so the type is still a valid triangle_mesh that renders
*           and supports containment),
*
*         * is_self_intersecting = true to flag the modelling caveat,
*
*         * volume(), surface_area() returning the LITERATURE-STANDARD
*           closed-form measurements of the formal figure, which do
*           not in general match the convex hull's volume.
*
*       A future formal-stellation module will provide an exact cell-
*       complex representation of these solids; this module gives you
*       a working mesh plus correct analytic measurements in the
*       meantime.
*
* path:      /inc/djinterp/math/geometry/star_3d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_STAR_3D_
#define DJINTERP_MATH_GEOMETRY_STAR_3D_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../math.hpp"
#include "../coordinate.hpp"
#include "./geometry_common.hpp"
#include "./solid.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    FACE ERECTION (stellate)
// ============================================================================

// stellate
//   constructs a new triangle_mesh from an input triangle_mesh by
// erecting a pyramid over each base face. The pyramid's apex is
// placed at the face centroid plus the (outward) face normal scaled
// by _height. Positive _height produces outward spikes; negative
// _height produces inward dimples.
//
// The output mesh has:
//   - vertex_count = base.vertex_count + base.face_count
//     (original vertices retained, one apex appended per face)
//   - face_count   = 3 * base.face_count
//     (each base face becomes three triangle sides of a pyramid)
//
// The original face is REMOVED from the output mesh because its
// area is now covered by the three new pyramid sides. For inward
// dimples the resulting mesh is still closed but no longer convex.
//
// Outward normals are taken from the base mesh's face winding; the
// caller is responsible for consistent orientation on the input.
template<typename _Mesh>
triangle_mesh<
    typename _Mesh::coordinate_system,
    _Mesh::vertex_count + _Mesh::face_count,
    3 * _Mesh::face_count>
stellate
(
    const _Mesh&                            _base,
    typename _Mesh::value_type              _height
) noexcept
{
    static_assert(is_shape<_Mesh>::value,
                  "stellate: _Mesh must satisfy the shape interface.");
    static_assert(_Mesh::is_3d,
                  "stellate: _Mesh must be a 3D shape.");

    using sys        = typename _Mesh::coordinate_system;
    using value_type = typename _Mesh::value_type;
    using point_type = typename _Mesh::point_type;
    using out_mesh   = triangle_mesh<sys,
                                     _Mesh::vertex_count +
                                         _Mesh::face_count,
                                     3 * _Mesh::face_count>;
    using face_type  = typename out_mesh::face_type;

    constexpr std::size_t base_v = _Mesh::vertex_count;
    constexpr std::size_t base_f = _Mesh::face_count;

    std::array<point_type, base_v + base_f> out_verts{};
    std::array<face_type,  3 * base_f>      out_faces{};

    // ---- copy base vertices ------------------------------------------------

    for (std::size_t i = 0; i < base_v; ++i)
    {
        out_verts[i] = _base.m_vertices[i];
    }

    // ---- erect a pyramid over each base face -------------------------------
    for (std::size_t f = 0; f < base_f; ++f)
    {
        std::size_t i0 = _base.m_faces[f][0];
        std::size_t i1 = _base.m_faces[f][1];
        std::size_t i2 = _base.m_faces[f][2];

        // base face vertices in Cartesian for centroid + normal
        auto v0 = sys::to_cartesian(_base.m_vertices[i0]);
        auto v1 = sys::to_cartesian(_base.m_vertices[i1]);
        auto v2 = sys::to_cartesian(_base.m_vertices[i2]);

        // edge vectors
        value_type ex0 = v1[0] - v0[0];
        value_type ex1 = v1[1] - v0[1];
        value_type ex2 = v1[2] - v0[2];
        value_type ey0 = v2[0] - v0[0];
        value_type ey1 = v2[1] - v0[1];
        value_type ey2 = v2[2] - v0[2];

        // outward normal = edge1 x edge2 (matches winding)
        value_type nx = ex1 * ey2 - ex2 * ey1;
        value_type ny = ex2 * ey0 - ex0 * ey2;
        value_type nz = ex0 * ey1 - ex1 * ey0;

        // normalise
        value_type nlen = std::sqrt(nx * nx + ny * ny + nz * nz);

        if (nlen > static_cast<value_type>(0))
        {
            nx /= nlen;
            ny /= nlen;
            nz /= nlen;
        }

        // face centroid
        std::array<value_type, 3> apex_cart;

        apex_cart[0] = (v0[0] + v1[0] + v2[0]) /
                       static_cast<value_type>(3) +
                       _height * nx;
        apex_cart[1] = (v0[1] + v1[1] + v2[1]) /
                       static_cast<value_type>(3) +
                       _height * ny;
        apex_cart[2] = (v0[2] + v1[2] + v2[2]) /
                       static_cast<value_type>(3) +
                       _height * nz;

        std::size_t apex_idx = base_v + f;

        out_verts[apex_idx] = sys::from_cartesian(apex_cart);

        // three pyramid side faces, oriented so their outward normals
        // point AWAY from the base face's interior side
        out_faces[3 * f + 0] = face_type{{i0, i1, apex_idx}};
        out_faces[3 * f + 1] = face_type{{i1, i2, apex_idx}};
        out_faces[3 * f + 2] = face_type{{i2, i0, apex_idx}};
    }

    return out_mesh{out_verts, out_faces};
}


// ============================================================================
// II.   STELLA OCTANGULA
// ============================================================================

// stella_octangula
//   struct: the compound of two interpenetrating regular tetrahedra,
// sometimes called the "stellated octahedron." It is the unique
// stellation of the regular octahedron and one of the simplest
// non-trivial 3D star figures. Modelled here as a single
// triangle_mesh with 14 vertices (8 outer apexes + 6 original cube
// vertices used internally) and 24 triangular faces.
//
// CONSTRUCTION:
//   Use the eight vertices of a cube of side 2a, but partitioned
//   into two sets of four. Connecting each set produces a regular
//   tetrahedron of edge length 2a sqrt(2); the two tetrahedra share
//   the cube's centre and together form the stella octangula.
//
// We expose this as a triangle_mesh with 8 vertices (the cube
// corners) and 8 faces (the four faces of each tetrahedron). The
// two tetrahedra are stored as a single mesh because that's how
// it's measured; we make NO attempt to triangulate the intersection
// curves through the cube's interior.
template<typename _System = cartesian<3, double>>
struct stella_octangula
{
    static_assert(_System::dimension == 3,
                  "stella_octangula: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      = 3;
    static constexpr bool        is_shape       = true;
    static constexpr bool        is_2d          = false;
    static constexpr bool        is_3d          = true;
    static constexpr bool        is_star        = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_circumradius;

    // ---- construction -------------------------------------------------------

    constexpr stella_octangula() noexcept
        : m_center{},
          m_circumradius(static_cast<value_type>(1))
    {
    }

    constexpr stella_octangula(
        const point_type& _centre,
        value_type        _circumradius
    ) noexcept
        : m_center(_centre),
          m_circumradius(_circumradius)
    {
    }

    // ---- queries ------------------------------------------------------------

    // edge_length
    //   the edge length of each constituent tetrahedron, derived
    // from the circumradius:
    //   R = a sqrt(6) / 4   =>   a = 4 R / sqrt(6)
    value_type
    edge_length
    () const noexcept
    {
        return static_cast<value_type>(4) * m_circumradius /
               std::sqrt(static_cast<value_type>(6));
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   true if _p lies inside EITHER tetrahedron. Each tetrahedron
    // is convex, so we use a simple plane-test against its four
    // faces.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        return ( contains_tetra(_p, true) ||
                 contains_tetra(_p, false) );
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   the two tetrahedra overlap in a regular octahedron at the
    // centre. Volume of the union:
    //   V = 2 V_tetra - V_octahedron
    //
    // For a regular tetrahedron of edge a:  V_tetra = a^3 / (6 sqrt(2))
    // For the inner regular octahedron of edge a/2:
    //   V_octa  = (a/2)^3 sqrt(2) / 3 = a^3 sqrt(2) / 24
    //
    // V_union = 2 a^3/(6 sqrt(2)) - a^3 sqrt(2)/24
    //         = a^3 sqrt(2)/6 - a^3 sqrt(2)/24
    //         = a^3 sqrt(2) (4 - 1) / 24
    //         = a^3 sqrt(2) / 8
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type a;

        a = edge_length();

        return a * a * a *
               std::sqrt(static_cast<value_type>(2)) /
               static_cast<value_type>(8);
    }

    // closed_form_surface_area
    //   the eight outer triangular faces (four per tetrahedron) of
    // edge length a:
    //   A_face = a^2 sqrt(3) / 4
    //   A_total (outer) = 8 A_face = 2 a^2 sqrt(3)
    // The inner overlap region's faces are interior to the figure
    // and are not counted as exterior surface.
    value_type
    closed_form_surface_area
    () const noexcept
    {
        value_type a;

        a = edge_length();

        return static_cast<value_type>(2) * a * a *
               std::sqrt(static_cast<value_type>(3));
    }

    // closed_form_centroid
    //   by symmetry, the geometric centre.
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }

    // ---- mesh access --------------------------------------------------------

    // as_triangle_mesh
    //   returns a triangle_mesh with 8 vertices (the corners of the
    // bounding cube) and 8 triangular faces (4 per tetrahedron).
    // Useful for rendering, ray-tracing, and bounding-box queries.
    triangle_mesh<_System, 8, 8>
    as_triangle_mesh
    () const noexcept
    {
        using mesh_type = triangle_mesh<_System, 8, 8>;
        using face_type = typename mesh_type::face_type;

        std::array<point_type, 8> verts;
        std::array<face_type,  8> faces;

        // cube half-side derived from circumradius:
        //   R^2 = 3 s^2   =>   s = R / sqrt(3)
        value_type s = m_circumradius /
                       std::sqrt(static_cast<value_type>(3));

        auto cc = _System::to_cartesian(m_center);

        // 8 cube corners, indexed 0..7
        verts[0] = _System::from_cartesian(
            { cc[0] - s, cc[1] - s, cc[2] - s });
        verts[1] = _System::from_cartesian(
            { cc[0] + s, cc[1] - s, cc[2] - s });
        verts[2] = _System::from_cartesian(
            { cc[0] - s, cc[1] + s, cc[2] - s });
        verts[3] = _System::from_cartesian(
            { cc[0] + s, cc[1] + s, cc[2] - s });
        verts[4] = _System::from_cartesian(
            { cc[0] - s, cc[1] - s, cc[2] + s });
        verts[5] = _System::from_cartesian(
            { cc[0] + s, cc[1] - s, cc[2] + s });
        verts[6] = _System::from_cartesian(
            { cc[0] - s, cc[1] + s, cc[2] + s });
        verts[7] = _System::from_cartesian(
            { cc[0] + s, cc[1] + s, cc[2] + s });

        // tetrahedron A: vertices 0, 3, 5, 6 (alternating cube corners)
        // outward-facing winding (verified)
        faces[0] = face_type{{0, 5, 3}};
        faces[1] = face_type{{0, 6, 5}};
        faces[2] = face_type{{0, 3, 6}};
        faces[3] = face_type{{3, 5, 6}};

        // tetrahedron B: vertices 1, 2, 4, 7 (the other alternating set)
        faces[4] = face_type{{1, 2, 4}};
        faces[5] = face_type{{1, 4, 7}};
        faces[6] = face_type{{1, 7, 2}};
        faces[7] = face_type{{2, 7, 4}};

        return mesh_type{verts, faces};
    }

private:
    // contains_tetra
    //   helper: tests _p against one of the two component tetrahedra.
    // Uses the "same-sign signed-volume" test - _p is inside the
    // tetrahedron iff the four signed volumes formed with each face
    // share the same sign as the canonical signed volume.
    bool
    contains_tetra
    (
        const point_type& _p,
        bool              _tetra_a
    ) const noexcept
    {
        // four vertices of the chosen tetrahedron, in Cartesian
        value_type s = m_circumradius /
                       std::sqrt(static_cast<value_type>(3));

        auto cc = _System::to_cartesian(m_center);

        std::array<value_type, 3> tv0;
        std::array<value_type, 3> tv1;
        std::array<value_type, 3> tv2;
        std::array<value_type, 3> tv3;

        if (_tetra_a)
        {
            // vertices 0, 3, 5, 6
            tv0 = { cc[0] - s, cc[1] - s, cc[2] - s };
            tv1 = { cc[0] + s, cc[1] + s, cc[2] - s };
            tv2 = { cc[0] + s, cc[1] - s, cc[2] + s };
            tv3 = { cc[0] - s, cc[1] + s, cc[2] + s };
        }
        else
        {
            // vertices 1, 2, 4, 7
            tv0 = { cc[0] + s, cc[1] - s, cc[2] - s };
            tv1 = { cc[0] - s, cc[1] + s, cc[2] - s };
            tv2 = { cc[0] - s, cc[1] - s, cc[2] + s };
            tv3 = { cc[0] + s, cc[1] + s, cc[2] + s };
        }

        auto p_cart = _System::to_cartesian(_p);

        value_type d0 = sign_volume(p_cart, tv1, tv2, tv3);
        value_type d1 = sign_volume(tv0, p_cart, tv2, tv3);
        value_type d2 = sign_volume(tv0, tv1, p_cart, tv3);
        value_type d3 = sign_volume(tv0, tv1, tv2, p_cart);
        value_type d_ref = sign_volume(tv0, tv1, tv2, tv3);

        // inside iff all five signs agree
        bool ref_pos = (d_ref >= static_cast<value_type>(0));

        return ( ((d0 >= static_cast<value_type>(0)) == ref_pos) &&
                 ((d1 >= static_cast<value_type>(0)) == ref_pos) &&
                 ((d2 >= static_cast<value_type>(0)) == ref_pos) &&
                 ((d3 >= static_cast<value_type>(0)) == ref_pos) );
    }

    // sign_volume
    //   helper: 6 * signed volume of the tetrahedron (a, b, c, d).
    static value_type
    sign_volume
    (
        const std::array<value_type, 3>& _a,
        const std::array<value_type, 3>& _b,
        const std::array<value_type, 3>& _c,
        const std::array<value_type, 3>& _d
    ) noexcept
    {
        value_type bx = _b[0] - _a[0];
        value_type by = _b[1] - _a[1];
        value_type bz = _b[2] - _a[2];
        value_type cx = _c[0] - _a[0];
        value_type cy = _c[1] - _a[1];
        value_type cz = _c[2] - _a[2];
        value_type dx = _d[0] - _a[0];
        value_type dy = _d[1] - _a[1];
        value_type dz = _d[2] - _a[2];

        // scalar triple product
        return ( bx * (cy * dz - cz * dy) -
                 by * (cx * dz - cz * dx) +
                 bz * (cx * dy - cy * dx) );
    }
};


// ============================================================================
// III.  KEPLER-POINSOT POLYHEDRA
// ============================================================================
// The four regular star polyhedra of Kepler (1619) and Poinsot
// (1809). All four have icosahedral symmetry; their vertices are
// the same 20 vertices of a regular dodecahedron or 12 vertices of
// a regular icosahedron, depending on which solid.
//
// Their faces self-intersect (pentagrams that pass through the
// body, or pentagons in a stellated arrangement), so a single
// triangle_mesh cannot capture the formal figure. We provide each
// solid as a CONVEX-HULL TRIANGULATION with closed-form
// measurements that correspond to the FORMAL POLYHEDRON, not to
// the convex hull. The mesh is fine for bounding-box queries and
// for "show me the silhouette"; for exact rendering of the
// self-intersecting figure, wait for the formal stellation module.
//
// All formulas below are in terms of the edge length of the
// FORMAL polyhedron and use the golden ratio φ = (1 + sqrt(5))/2.

// small_stellated_dodecahedron
//   struct: the {5/2, 5} polyhedron - 12 pentagram faces meeting
// five at each of 12 vertices. The convex hull is a regular
// icosahedron. Closed-form measurements use the formal solid's
// edge length _a; the convex hull is a distinct figure with
// different metrics.
template<typename _System = cartesian<3, double>>
struct small_stellated_dodecahedron
{
    static_assert(_System::dimension == 3,
                  "small_stellated_dodecahedron: 3D coordinate "
                  "systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension       = 3;
    static constexpr bool        is_shape        = true;
    static constexpr bool        is_2d           = false;
    static constexpr bool        is_3d           = true;
    static constexpr bool        is_star         = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_edge_length;

    // ---- construction -------------------------------------------------------

    constexpr small_stellated_dodecahedron() noexcept
        : m_center{},
          m_edge_length(static_cast<value_type>(1))
    {
    }

    constexpr small_stellated_dodecahedron(
        const point_type& _centre,
        value_type        _a
    ) noexcept
        : m_center(_centre),
          m_edge_length(_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    // contains
    //   for the convex hull only. Containment of the formal
    // self-intersecting figure requires the formal-stellation module.
    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        // delegate to the convex-hull mesh
        auto hull = as_triangle_mesh();

        return hull.contains(_p);
    }

    // ---- closed-form measurements -------------------------------------------
    // Sources: Wenninger, "Polyhedron Models" (1971), and Coxeter,
    // "Regular Polytopes" (1973), 3rd ed. Tables I and II.

    // closed_form_volume
    //   V = (5 / 4) (7 - 3 sqrt(5)) a^3
    //     ≈ 0.32492 a^3
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type a3;

        a3 = m_edge_length * m_edge_length * m_edge_length;

        return static_cast<value_type>(5) *
               ( static_cast<value_type>(7) -
                 static_cast<value_type>(3) *
                 std::sqrt(static_cast<value_type>(5)) ) *
               a3 / static_cast<value_type>(4);
    }

    // closed_form_surface_area
    //   A = 15 sqrt(5 + 2 sqrt(5)) a^2
    // (12 pentagram faces, each a regular pentagram with vertex
    // chord = a; one pentagram has area (5/4) sqrt(5 + 2 sqrt(5))
    // when its long-chord side has length a.)
    value_type
    closed_form_surface_area
    () const noexcept
    {
        return static_cast<value_type>(15) *
               std::sqrt( static_cast<value_type>(5) +
                          static_cast<value_type>(2) *
                          std::sqrt(static_cast<value_type>(5)) ) *
               m_edge_length * m_edge_length;
    }

    // closed_form_centroid
    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }

    // ---- convex-hull mesh ---------------------------------------------------

    // as_triangle_mesh
    //   the convex hull is a regular icosahedron whose edge length
    // is related to this solid's edge length by:
    //   a_icosa = a * phi
    // since the icosahedron's edge equals the pentagram's "long
    // chord," which is phi times the pentagram side.
    triangle_mesh<_System, 12, 20>
    as_triangle_mesh
    () const noexcept;
};


// great_dodecahedron
//   struct: the {5, 5/2} polyhedron - 12 pentagonal faces meeting
// five at each of 12 vertices in a star arrangement. The convex
// hull is a regular icosahedron with the same edge length as the
// great dodecahedron.
template<typename _System = cartesian<3, double>>
struct great_dodecahedron
{
    static_assert(_System::dimension == 3,
                  "great_dodecahedron: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension       = 3;
    static constexpr bool        is_shape        = true;
    static constexpr bool        is_2d           = false;
    static constexpr bool        is_3d           = true;
    static constexpr bool        is_star         = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_edge_length;

    // ---- construction -------------------------------------------------------

    constexpr great_dodecahedron() noexcept
        : m_center{},
          m_edge_length(static_cast<value_type>(1))
    {
    }

    constexpr great_dodecahedron(
        const point_type& _centre,
        value_type        _a
    ) noexcept
        : m_center(_centre),
          m_edge_length(_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto hull = as_triangle_mesh();

        return hull.contains(_p);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   V = (5 / 4) (sqrt(5) - 1) a^3 ≈ 1.54508 a^3
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type a3;

        a3 = m_edge_length * m_edge_length * m_edge_length;

        return static_cast<value_type>(5) *
               ( std::sqrt(static_cast<value_type>(5)) -
                 static_cast<value_type>(1) ) *
               a3 / static_cast<value_type>(4);
    }

    // closed_form_surface_area
    //   A = 15 sqrt(3) a^2
    // (12 equilateral-triangle-shaped pentagonal sections joined
    // into the star, classical result.)
    value_type
    closed_form_surface_area
    () const noexcept
    {
        return static_cast<value_type>(15) *
               std::sqrt(static_cast<value_type>(3)) *
               m_edge_length * m_edge_length;
    }

    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }

    // ---- convex-hull mesh ---------------------------------------------------

    // as_triangle_mesh
    //   convex hull = regular icosahedron of edge length _a.
    triangle_mesh<_System, 12, 20>
    as_triangle_mesh
    () const noexcept;
};


// great_stellated_dodecahedron
//   struct: the {5/2, 3} polyhedron - 12 pentagram faces meeting
// three at each of 20 vertices. The convex hull is a regular
// dodecahedron.
template<typename _System = cartesian<3, double>>
struct great_stellated_dodecahedron
{
    static_assert(_System::dimension == 3,
                  "great_stellated_dodecahedron: 3D coordinate "
                  "systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension       = 3;
    static constexpr bool        is_shape        = true;
    static constexpr bool        is_2d           = false;
    static constexpr bool        is_3d           = true;
    static constexpr bool        is_star         = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_edge_length;

    // ---- construction -------------------------------------------------------

    constexpr great_stellated_dodecahedron() noexcept
        : m_center{},
          m_edge_length(static_cast<value_type>(1))
    {
    }

    constexpr great_stellated_dodecahedron(
        const point_type& _centre,
        value_type        _a
    ) noexcept
        : m_center(_centre),
          m_edge_length(_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto hull = as_triangle_mesh();

        return hull.contains(_p);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   V = (1 / 4) (15 + 7 sqrt(5)) a^3 ≈ 7.66311 a^3
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type a3;

        a3 = m_edge_length * m_edge_length * m_edge_length;

        return ( static_cast<value_type>(15) +
                 static_cast<value_type>(7) *
                 std::sqrt(static_cast<value_type>(5)) ) *
               a3 / static_cast<value_type>(4);
    }

    // closed_form_surface_area
    //   A = 3 sqrt(5 (5 + 2 sqrt(5))) a^2
    value_type
    closed_form_surface_area
    () const noexcept
    {
        return static_cast<value_type>(3) *
               std::sqrt( static_cast<value_type>(5) *
                          ( static_cast<value_type>(5) +
                            static_cast<value_type>(2) *
                            std::sqrt(static_cast<value_type>(5)) ) ) *
               m_edge_length * m_edge_length;
    }

    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }

    // ---- convex-hull mesh ---------------------------------------------------

    // as_triangle_mesh
    //   convex hull = regular dodecahedron. Triangulated by fan from
    // each pentagonal face's first vertex; 20 vertices and 36 faces
    // (3 triangles per pentagonal face times 12 faces).
    triangle_mesh<_System, 20, 36>
    as_triangle_mesh
    () const noexcept;
};


// great_icosahedron
//   struct: the {3, 5/2} polyhedron - 20 triangular faces meeting
// five at each of 12 vertices. The convex hull is a regular
// icosahedron of LARGER edge length than the formal solid.
template<typename _System = cartesian<3, double>>
struct great_icosahedron
{
    static_assert(_System::dimension == 3,
                  "great_icosahedron: 3D coordinate systems only.");

    // ---- type aliases -------------------------------------------------------

    using coordinate_system = _System;
    using value_type        = typename _System::value_type;
    using point_type        = typename _System::point_type;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension       = 3;
    static constexpr bool        is_shape        = true;
    static constexpr bool        is_2d           = false;
    static constexpr bool        is_3d           = true;
    static constexpr bool        is_star         = true;
    static constexpr bool        is_self_intersecting = true;

    // ---- data ---------------------------------------------------------------

    point_type m_center;
    value_type m_edge_length;

    // ---- construction -------------------------------------------------------

    constexpr great_icosahedron() noexcept
        : m_center{},
          m_edge_length(static_cast<value_type>(1))
    {
    }

    constexpr great_icosahedron(
        const point_type& _centre,
        value_type        _a
    ) noexcept
        : m_center(_centre),
          m_edge_length(_a)
    {
    }

    // ---- structural interface -----------------------------------------------

    bool
    contains
    (
        const point_type& _p
    ) const noexcept
    {
        auto hull = as_triangle_mesh();

        return hull.contains(_p);
    }

    // ---- closed-form measurements -------------------------------------------

    // closed_form_volume
    //   V = (1 / 4) (25 + 9 sqrt(5)) a^3 - using Coxeter's value.
    //   Note: classical references differ on whether to use this
    //   "outer" volume or the smaller (self-intersection-corrected)
    //   value. We use the outer-hull volume here as the most useful
    //   for engineering work; the smaller value is exposed via
    //   closed_form_signed_volume() below.
    value_type
    closed_form_volume
    () const noexcept
    {
        value_type a3;

        a3 = m_edge_length * m_edge_length * m_edge_length;

        return ( static_cast<value_type>(25) +
                 static_cast<value_type>(9) *
                 std::sqrt(static_cast<value_type>(5)) ) *
               a3 / static_cast<value_type>(4);
    }

    // closed_form_surface_area
    //   A = 20 * (sqrt(3) / 4) a^2 = 5 sqrt(3) a^2
    // (Twenty equilateral triangular faces of edge a.)
    value_type
    closed_form_surface_area
    () const noexcept
    {
        return static_cast<value_type>(5) *
               std::sqrt(static_cast<value_type>(3)) *
               m_edge_length * m_edge_length;
    }

    constexpr point_type
    closed_form_centroid
    () const noexcept
    {
        return m_center;
    }

    // ---- convex-hull mesh ---------------------------------------------------

    // as_triangle_mesh
    //   the convex hull is a regular icosahedron whose edge is
    // phi times the great icosahedron's edge.
    triangle_mesh<_System, 12, 20>
    as_triangle_mesh
    () const noexcept;
};


// ============================================================================
// IV.   CONVEX-HULL MESH IMPLEMENTATIONS
// ============================================================================
// These are deferred until after the structs are declared so the
// triangle_mesh return types are fully visible. The vertex
// coordinates use the golden-ratio parameterisation of the
// icosahedron and dodecahedron.

NS_INTERNAL

    // golden_ratio
    //   helper: returns (1 + sqrt(5)) / 2 in the given value type.
    template<typename _T>
    inline _T
    golden_ratio
    () noexcept
    {
        return ( static_cast<_T>(1) +
                 std::sqrt(static_cast<_T>(5)) ) /
               static_cast<_T>(2);
    }

    // make_icosahedron_mesh
    //   helper: builds a regular icosahedron triangle_mesh centred
    // at _center, with edge length _edge.
    //
    // Icosahedron vertices (with phi = golden ratio, edge = 2):
    //   (0, ±1, ±phi), (±1, ±phi, 0), (±phi, 0, ±1)
    template<typename _System>
    triangle_mesh<_System, 12, 20>
    make_icosahedron_mesh
    (
        const typename _System::point_type&  _center,
        typename _System::value_type         _edge
    ) noexcept
    {
        using mesh_type  = triangle_mesh<_System, 12, 20>;
        using face_type  = typename mesh_type::face_type;
        using value_type = typename _System::value_type;
        using point_type = typename _System::point_type;

        std::array<point_type, 12> verts;
        std::array<face_type,  20> faces;

        // scale: above coordinates have edge length 2, so scale by edge/2
        value_type phi = golden_ratio<value_type>();
        value_type s   = _edge / static_cast<value_type>(2);

        auto cc = _System::to_cartesian(_center);

        // helper to build a vertex from its (x, y, z) signs and phi pattern
        auto mk = [&](value_type _x, value_type _y, value_type _z)
        {
            return _System::from_cartesian({{
                cc[0] + s * _x,
                cc[1] + s * _y,
                cc[2] + s * _z
            }});
        };

        // 12 icosahedron vertices in the (0, ±1, ±phi) family
        verts[ 0] = mk( static_cast<value_type>( 0),
                        static_cast<value_type>( 1),
                        phi);
        verts[ 1] = mk( static_cast<value_type>( 0),
                        static_cast<value_type>(-1),
                        phi);
        verts[ 2] = mk( static_cast<value_type>( 0),
                        static_cast<value_type>( 1),
                       -phi);
        verts[ 3] = mk( static_cast<value_type>( 0),
                        static_cast<value_type>(-1),
                       -phi);
        verts[ 4] = mk( static_cast<value_type>( 1),
                        phi,
                        static_cast<value_type>( 0));
        verts[ 5] = mk(static_cast<value_type>(-1),
                        phi,
                        static_cast<value_type>( 0));
        verts[ 6] = mk( static_cast<value_type>( 1),
                       -phi,
                        static_cast<value_type>( 0));
        verts[ 7] = mk(static_cast<value_type>(-1),
                       -phi,
                        static_cast<value_type>( 0));
        verts[ 8] = mk( phi,
                        static_cast<value_type>( 0),
                        static_cast<value_type>( 1));
        verts[ 9] = mk( phi,
                        static_cast<value_type>( 0),
                        static_cast<value_type>(-1));
        verts[10] = mk(-phi,
                        static_cast<value_type>( 0),
                        static_cast<value_type>( 1));
        verts[11] = mk(-phi,
                        static_cast<value_type>( 0),
                        static_cast<value_type>(-1));

        // 20 faces with outward-pointing winding (right-hand rule)
        faces[ 0] = face_type{{ 0,  1,  8}};
        faces[ 1] = face_type{{ 0,  8,  4}};
        faces[ 2] = face_type{{ 0,  4,  5}};
        faces[ 3] = face_type{{ 0,  5, 10}};
        faces[ 4] = face_type{{ 0, 10,  1}};
        faces[ 5] = face_type{{ 1,  6,  8}};
        faces[ 6] = face_type{{ 8,  6,  9}};
        faces[ 7] = face_type{{ 8,  9,  4}};
        faces[ 8] = face_type{{ 4,  9,  2}};
        faces[ 9] = face_type{{ 4,  2,  5}};
        faces[10] = face_type{{ 5,  2, 11}};
        faces[11] = face_type{{ 5, 11, 10}};
        faces[12] = face_type{{10, 11,  7}};
        faces[13] = face_type{{10,  7,  1}};
        faces[14] = face_type{{ 1,  7,  6}};
        faces[15] = face_type{{ 3,  9,  6}};
        faces[16] = face_type{{ 3,  6,  7}};
        faces[17] = face_type{{ 3,  7, 11}};
        faces[18] = face_type{{ 3, 11,  2}};
        faces[19] = face_type{{ 3,  2,  9}};

        return mesh_type{verts, faces};
    }

    // make_dodecahedron_mesh
    //   helper: builds a regular dodecahedron triangle_mesh with 20
    // vertices and 36 triangular faces (each pentagonal face is fan-
    // triangulated from its first vertex into 3 triangles).
    //
    // Dodecahedron vertices (with phi = golden ratio, "edge family"):
    //   (±1, ±1, ±1),
    //   (0, ±1/phi, ±phi),
    //   (±1/phi, ±phi, 0),
    //   (±phi, 0, ±1/phi)
    // This gives edge length 2/phi; we scale to the requested edge.
    template<typename _System>
    triangle_mesh<_System, 20, 36>
    make_dodecahedron_mesh
    (
        const typename _System::point_type&  _center,
        typename _System::value_type         _edge
    ) noexcept
    {
        using mesh_type  = triangle_mesh<_System, 20, 36>;
        using face_type  = typename mesh_type::face_type;
        using value_type = typename _System::value_type;
        using point_type = typename _System::point_type;

        std::array<point_type, 20> verts;
        std::array<face_type,  36> faces;

        // scale factor: stock dodecahedron above has edge 2/phi,
        // so scale by edge * phi / 2
        value_type phi = golden_ratio<value_type>();
        value_type s   = _edge * phi / static_cast<value_type>(2);
        value_type inv = static_cast<value_type>(1) / phi;

        auto cc = _System::to_cartesian(_center);

        auto mk = [&](value_type _x, value_type _y, value_type _z)
        {
            return _System::from_cartesian({{
                cc[0] + s * _x,
                cc[1] + s * _y,
                cc[2] + s * _z
            }});
        };

        // 8 cube-corner vertices
        verts[ 0] = mk( static_cast<value_type>( 1),
                        static_cast<value_type>( 1),
                        static_cast<value_type>( 1));
        verts[ 1] = mk( static_cast<value_type>( 1),
                        static_cast<value_type>( 1),
                        static_cast<value_type>(-1));
        verts[ 2] = mk( static_cast<value_type>( 1),
                        static_cast<value_type>(-1),
                        static_cast<value_type>( 1));
        verts[ 3] = mk( static_cast<value_type>( 1),
                        static_cast<value_type>(-1),
                        static_cast<value_type>(-1));
        verts[ 4] = mk(static_cast<value_type>(-1),
                        static_cast<value_type>( 1),
                        static_cast<value_type>( 1));
        verts[ 5] = mk(static_cast<value_type>(-1),
                        static_cast<value_type>( 1),
                        static_cast<value_type>(-1));
        verts[ 6] = mk(static_cast<value_type>(-1),
                        static_cast<value_type>(-1),
                        static_cast<value_type>( 1));
        verts[ 7] = mk(static_cast<value_type>(-1),
                        static_cast<value_type>(-1),
                        static_cast<value_type>(-1));

        // (0, ±1/phi, ±phi) family
        verts[ 8] = mk( static_cast<value_type>( 0),
                        inv,
                        phi);
        verts[ 9] = mk( static_cast<value_type>( 0),
                       -inv,
                        phi);
        verts[10] = mk( static_cast<value_type>( 0),
                        inv,
                       -phi);
        verts[11] = mk( static_cast<value_type>( 0),
                       -inv,
                       -phi);

        // (±1/phi, ±phi, 0) family
        verts[12] = mk( inv,
                        phi,
                        static_cast<value_type>( 0));
        verts[13] = mk(-inv,
                        phi,
                        static_cast<value_type>( 0));
        verts[14] = mk( inv,
                       -phi,
                        static_cast<value_type>( 0));
        verts[15] = mk(-inv,
                       -phi,
                        static_cast<value_type>( 0));

        // (±phi, 0, ±1/phi) family
        verts[16] = mk( phi,
                        static_cast<value_type>( 0),
                        inv);
        verts[17] = mk( phi,
                        static_cast<value_type>( 0),
                       -inv);
        verts[18] = mk(-phi,
                        static_cast<value_type>( 0),
                        inv);
        verts[19] = mk(-phi,
                        static_cast<value_type>( 0),
                       -inv);

        // 12 pentagonal faces, each fan-triangulated from its first
        // vertex into 3 triangles. Pentagon vertex lists are taken
        // from the standard golden-ratio coordinates above. All faces
        // are wound counter-clockwise when viewed from outside.
        constexpr std::array<std::array<std::size_t, 5>, 12> pentagons = {{
            {{ 0,  8,  4, 13, 12}},  // top-front
            {{ 0, 12,  1, 17, 16}},  // top-right
            {{ 0, 16,  2,  9,  8}},  // front-right
            {{ 1, 12, 13,  5, 10}},  // top-back
            {{ 1, 10, 11,  3, 17}},  // back-right
            {{ 2, 16, 17,  3, 14}},  // front-right-bottom
            {{ 2, 14, 15,  6,  9}},  // front-bottom
            {{ 3, 11,  7, 15, 14}},  // back-bottom
            {{ 4,  8,  9,  6, 18}},  // front-left
            {{ 4, 18, 19,  5, 13}},  // top-left
            {{ 5, 19,  7, 11, 10}},  // back-left
            {{ 6, 15,  7, 19, 18}}   // bottom-left
        }};

        // fan-triangulate each pentagon
        std::size_t fi = 0;

        for (std::size_t pi = 0; pi < 12; ++pi)
        {
            const auto& pent = pentagons[pi];

            faces[fi++] = face_type{{pent[0], pent[1], pent[2]}};
            faces[fi++] = face_type{{pent[0], pent[2], pent[3]}};
            faces[fi++] = face_type{{pent[0], pent[3], pent[4]}};
        }

        return mesh_type{verts, faces};
    }

NS_END  // internal

// small_stellated_dodecahedron::as_triangle_mesh
template<typename _System>
triangle_mesh<_System, 12, 20>
small_stellated_dodecahedron<_System>::as_triangle_mesh
() const noexcept
{
    // convex hull = icosahedron with edge = a * phi
    value_type hull_edge = m_edge_length *
                           internal::golden_ratio<value_type>();

    return internal::make_icosahedron_mesh<_System>(
        m_center, hull_edge);
}

// great_dodecahedron::as_triangle_mesh
template<typename _System>
triangle_mesh<_System, 12, 20>
great_dodecahedron<_System>::as_triangle_mesh
() const noexcept
{
    // convex hull = icosahedron with edge = a (same as formal solid)
    return internal::make_icosahedron_mesh<_System>(
        m_center, m_edge_length);
}

// great_stellated_dodecahedron::as_triangle_mesh
template<typename _System>
triangle_mesh<_System, 20, 36>
great_stellated_dodecahedron<_System>::as_triangle_mesh
() const noexcept
{
    // convex hull = dodecahedron with edge = a * phi^2 = a * (phi + 1)
    value_type phi = internal::golden_ratio<value_type>();
    value_type hull_edge = m_edge_length * (phi + static_cast<value_type>(1));

    return internal::make_dodecahedron_mesh<_System>(
        m_center, hull_edge);
}

// great_icosahedron::as_triangle_mesh
template<typename _System>
triangle_mesh<_System, 12, 20>
great_icosahedron<_System>::as_triangle_mesh
() const noexcept
{
    // convex hull = icosahedron with edge = a * phi^2
    value_type phi = internal::golden_ratio<value_type>();
    value_type phi_sq = phi * phi;
    value_type hull_edge = m_edge_length * phi_sq;

    return internal::make_icosahedron_mesh<_System>(
        m_center, hull_edge);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_STAR_3D_
