/******************************************************************************
* djinterp [math]                                                 platonic.hpp
*
* The five Platonic solids as explicit triangle meshes.
*   Each factory returns a triangle_mesh<cartesian<3, _T>, V, F> whose
* vertices lie on a sphere of the requested circumradius, centred at the
* origin.  Because they are triangle_mesh instances they inherit closed-form
* volume (divergence theorem), surface area, centroid, and ray-cast
* containment from solid.hpp at no extra cost, and they feed a renderer's mesh
* adapter directly (m_vertices / m_faces).
*
*   Faces are oriented outward automatically (each triangle's winding is
* chosen so its normal points away from the origin), so the factories do not
* depend on hand-specified winding.
*
* PROVIDED FACTORIES:
*   tetrahedron_mesh<_T>(radius)    -  4 vertices,  4 faces
*   hexahedron_mesh<_T>(radius)     -  8 vertices, 12 faces   (the cube)
*   octahedron_mesh<_T>(radius)     -  6 vertices,  8 faces
*   dodecahedron_mesh<_T>(radius)   - 20 vertices, 36 faces   (12 pentagons)
*   icosahedron_mesh<_T>(radius)    - 12 vertices, 20 faces
*
* path:      /inc/djinterp/math/geometry/platonic.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.06.18
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_PLATONIC_
#define DJINTERP_MATH_GEOMETRY_PLATONIC_ 1

#include <cstddef>
#include <array>
#include "../../djinterp.hpp"
#include "../coordinate/coordinate.hpp"
#include "./geometry_common.hpp"
#include "./solid.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    INTERNAL ORIENTATION HELPERS
// ============================================================================

NS_INTERNAL

    // platonic_sub
    //   helper: component-wise difference of two Cartesian triples.
    template<typename _T>
    std::array<_T, 3>
    platonic_sub
    (
        const std::array<_T, 3>& _a,
        const std::array<_T, 3>& _b
    ) noexcept
    {
        return {{ _a[0] - _b[0],
                  _a[1] - _b[1],
                  _a[2] - _b[2] }};
    }

    // platonic_cross
    //   helper: cross product of two Cartesian triples.
    template<typename _T>
    std::array<_T, 3>
    platonic_cross
    (
        const std::array<_T, 3>& _a,
        const std::array<_T, 3>& _b
    ) noexcept
    {
        return {{ _a[1] * _b[2] - _a[2] * _b[1],
                  _a[2] * _b[0] - _a[0] * _b[2],
                  _a[0] * _b[1] - _a[1] * _b[0] }};
    }

    // platonic_dot
    //   helper: dot product of two Cartesian triples.
    template<typename _T>
    _T
    platonic_dot
    (
        const std::array<_T, 3>& _a,
        const std::array<_T, 3>& _b
    ) noexcept
    {
        return _a[0] * _b[0] +
               _a[1] * _b[1] +
               _a[2] * _b[2];
    }

    // outward_triangle
    //   helper: returns the triple (_i, _j, _k) reordered so that its outward
    // normal points away from the origin (valid for an origin-centred solid).
    template<typename    _T,
             std::size_t _V>
    std::array<std::size_t, 3>
    outward_triangle
    (
        const std::array<std::array<_T, 3>, _V>& _verts,
        std::size_t                              _i,
        std::size_t                              _j,
        std::size_t                              _k
    ) noexcept
    {
        std::array<_T, 3> e1 = platonic_sub(_verts[_j], _verts[_i]);
        std::array<_T, 3> e2 = platonic_sub(_verts[_k], _verts[_i]);
        std::array<_T, 3> n  = platonic_cross(e1, e2);

        // face centre (proportional to the sum of the three vertices)
        std::array<_T, 3> c = {{ _verts[_i][0] + _verts[_j][0] + _verts[_k][0],
                                 _verts[_i][1] + _verts[_j][1] + _verts[_k][1],
                                 _verts[_i][2] + _verts[_j][2] + _verts[_k][2] }};

        if (platonic_dot(n, c) < static_cast<_T>(0))
        {
            return {{ _i, _k, _j }};
        }

        return {{ _i, _j, _k }};
    }

    // orient_triangles
    //   helper: orients every triangle of _raw outward.
    template<typename    _T,
             std::size_t _V,
             std::size_t _F>
    std::array<std::array<std::size_t, 3>, _F>
    orient_triangles
    (
        const std::array<std::array<_T, 3>, _V>&          _verts,
        const std::array<std::array<std::size_t, 3>, _F>& _raw
    ) noexcept
    {
        std::array<std::array<std::size_t, 3>, _F> out{};

        for (std::size_t f = 0; f < _F; ++f)
        {
            out[f] = outward_triangle(_verts,
                                      _raw[f][0],
                                      _raw[f][1],
                                      _raw[f][2]);
        }

        return out;
    }

NS_END  // internal


// ============================================================================
// II.   TETRAHEDRON
// ============================================================================

// tetrahedron_mesh
//   the regular tetrahedron inscribed in a sphere of radius _radius.
template<typename _T = double>
triangle_mesh<cartesian<3, _T>, 4, 4>
tetrahedron_mesh
(
    _T _radius = static_cast<_T>(1)
) noexcept
{
    using mesh_type  = triangle_mesh<cartesian<3, _T>, 4, 4>;
    using point_type = typename mesh_type::point_type;
    using face_type  = typename mesh_type::face_type;

    // canonical circumradius is sqrt(3); scale onto the requested sphere
    _T s = _radius * static_cast<_T>(0.5773502691896258);

    std::array<point_type, 4> verts = {{
        {{  s,  s,  s }},
        {{  s, -s, -s }},
        {{ -s,  s, -s }},
        {{ -s, -s,  s }}
    }};

    std::array<face_type, 4> raw = {{
        {{ 0, 1, 2 }},
        {{ 0, 1, 3 }},
        {{ 0, 2, 3 }},
        {{ 1, 2, 3 }}
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}


// ============================================================================
// III.  HEXAHEDRON (CUBE)
// ============================================================================

// hexahedron_mesh
//   the cube inscribed in a sphere of radius _radius, each square face split
// into two triangles.
template<typename _T = double>
triangle_mesh<cartesian<3, _T>, 8, 12>
hexahedron_mesh
(
    _T _radius = static_cast<_T>(1)
) noexcept
{
    using mesh_type  = triangle_mesh<cartesian<3, _T>, 8, 12>;
    using point_type = typename mesh_type::point_type;
    using face_type  = typename mesh_type::face_type;

    // canonical circumradius is sqrt(3)
    _T s = _radius * static_cast<_T>(0.5773502691896258);

    std::array<point_type, 8> verts = {{
        {{ -s, -s, -s }},   // 0
        {{  s, -s, -s }},   // 1
        {{  s,  s, -s }},   // 2
        {{ -s,  s, -s }},   // 3
        {{ -s, -s,  s }},   // 4
        {{  s, -s,  s }},   // 5
        {{  s,  s,  s }},   // 6
        {{ -s,  s,  s }}    // 7
    }};

    std::array<face_type, 12> raw = {{
        {{ 0, 1, 2 }}, {{ 0, 2, 3 }},   // z = -s
        {{ 4, 5, 6 }}, {{ 4, 6, 7 }},   // z = +s
        {{ 0, 1, 5 }}, {{ 0, 5, 4 }},   // y = -s
        {{ 3, 2, 6 }}, {{ 3, 6, 7 }},   // y = +s
        {{ 0, 3, 7 }}, {{ 0, 7, 4 }},   // x = -s
        {{ 1, 2, 6 }}, {{ 1, 6, 5 }}    // x = +s
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}


// ============================================================================
// IV.   OCTAHEDRON
// ============================================================================

// octahedron_mesh
//   the regular octahedron inscribed in a sphere of radius _radius (the
// vertices sit on the axes).
template<typename _T = double>
triangle_mesh<cartesian<3, _T>, 6, 8>
octahedron_mesh
(
    _T _radius = static_cast<_T>(1)
) noexcept
{
    using mesh_type  = triangle_mesh<cartesian<3, _T>, 6, 8>;
    using point_type = typename mesh_type::point_type;
    using face_type  = typename mesh_type::face_type;

    // canonical circumradius is 1
    _T s = _radius;
    _T z = static_cast<_T>(0);

    std::array<point_type, 6> verts = {{
        {{  s,  z,  z }},   // 0
        {{ -s,  z,  z }},   // 1
        {{  z,  s,  z }},   // 2
        {{  z, -s,  z }},   // 3
        {{  z,  z,  s }},   // 4
        {{  z,  z, -s }}    // 5
    }};

    std::array<face_type, 8> raw = {{
        {{ 0, 2, 4 }}, {{ 0, 2, 5 }}, {{ 0, 3, 4 }}, {{ 0, 3, 5 }},
        {{ 1, 2, 4 }}, {{ 1, 2, 5 }}, {{ 1, 3, 4 }}, {{ 1, 3, 5 }}
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}


// ============================================================================
// V.    ICOSAHEDRON
// ============================================================================

// icosahedron_mesh
//   the regular icosahedron inscribed in a sphere of radius _radius.  The
// twelve vertices are the cyclic permutations of (0, +/-1, +/-phi).
template<typename _T = double>
triangle_mesh<cartesian<3, _T>, 12, 20>
icosahedron_mesh
(
    _T _radius = static_cast<_T>(1)
) noexcept
{
    using mesh_type  = triangle_mesh<cartesian<3, _T>, 12, 20>;
    using point_type = typename mesh_type::point_type;
    using face_type  = typename mesh_type::face_type;

    _T phi = static_cast<_T>(1.6180339887498949);

    // canonical circumradius is sqrt(1 + phi^2)
    _T s = _radius * static_cast<_T>(0.5257311121191336);

    _T a = s;
    _T b = s * phi;
    _T z = static_cast<_T>(0);

    std::array<point_type, 12> verts = {{
        {{ -a,  b,  z }},   // 0
        {{  a,  b,  z }},   // 1
        {{ -a, -b,  z }},   // 2
        {{  a, -b,  z }},   // 3
        {{  z, -a,  b }},   // 4
        {{  z,  a,  b }},   // 5
        {{  z, -a, -b }},   // 6
        {{  z,  a, -b }},   // 7
        {{  b,  z, -a }},   // 8
        {{  b,  z,  a }},   // 9
        {{ -b,  z, -a }},   // 10
        {{ -b,  z,  a }}    // 11
    }};

    std::array<face_type, 20> raw = {{
        {{ 0, 11,  5 }}, {{ 0,  5,  1 }}, {{ 0,  1,  7 }},
        {{ 0,  7, 10 }}, {{ 0, 10, 11 }},
        {{ 1,  5,  9 }}, {{ 5, 11,  4 }}, {{ 11, 10,  2 }},
        {{ 10, 7,  6 }}, {{ 7,  1,  8 }},
        {{ 3,  9,  4 }}, {{ 3,  4,  2 }}, {{ 3,  2,  6 }},
        {{ 3,  6,  8 }}, {{ 3,  8,  9 }},
        {{ 4,  9,  5 }}, {{ 2,  4, 11 }}, {{ 6,  2, 10 }},
        {{ 8,  6,  7 }}, {{ 9,  8,  1 }}
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}


// ============================================================================
// VI.   DODECAHEDRON
// ============================================================================

// dodecahedron_mesh
//   the regular dodecahedron inscribed in a sphere of radius _radius.  Its
// twenty vertices are the cube corners (+/-1, +/-1, +/-1) together with the
// cyclic permutations of (0, +/-1/phi, +/-phi).  Each of the twelve
// pentagonal faces is fan-triangulated into three triangles (36 total).
template<typename _T = double>
triangle_mesh<cartesian<3, _T>, 20, 36>
dodecahedron_mesh
(
    _T _radius = static_cast<_T>(1)
) noexcept
{
    using mesh_type  = triangle_mesh<cartesian<3, _T>, 20, 36>;
    using point_type = typename mesh_type::point_type;
    using face_type  = typename mesh_type::face_type;

    _T phi = static_cast<_T>(1.6180339887498949);
    _T inv = static_cast<_T>(0.6180339887498949);

    // canonical circumradius is sqrt(3)
    _T s  = _radius * static_cast<_T>(0.5773502691896258);
    _T c  = s;
    _T pj = s * phi;
    _T ij = s * inv;
    _T z  = static_cast<_T>(0);

    std::array<point_type, 20> verts = {{
        {{  c,  c,  c }},    // 0
        {{  c,  c, -c }},    // 1
        {{  c, -c,  c }},    // 2
        {{  c, -c, -c }},    // 3
        {{ -c,  c,  c }},    // 4
        {{ -c,  c, -c }},    // 5
        {{ -c, -c,  c }},    // 6
        {{ -c, -c, -c }},    // 7
        {{  z,  ij,  pj }},  // 8
        {{  z,  ij, -pj }},  // 9
        {{  z, -ij,  pj }},  // 10
        {{  z, -ij, -pj }},  // 11
        {{  ij,  pj,  z }},  // 12
        {{  ij, -pj,  z }},  // 13
        {{ -ij,  pj,  z }},  // 14
        {{ -ij, -pj,  z }},  // 15
        {{  pj,  z,  ij }},  // 16
        {{  pj,  z, -ij }},  // 17
        {{ -pj,  z,  ij }},  // 18
        {{ -pj,  z, -ij }}   // 19
    }};

    // the twelve pentagonal faces, in cycle order (winding fixed below)
    std::array<std::array<std::size_t, 5>, 12> pents = {{
        {{  8,  0, 16,  2, 10 }},
        {{  8,  0, 12, 14,  4 }},
        {{  8,  4, 18,  6, 10 }},
        {{  0, 12,  1, 17, 16 }},
        {{ 16,  2, 13,  3, 17 }},
        {{  2, 10,  6, 15, 13 }},
        {{ 11,  7, 19,  5,  9 }},
        {{ 11,  7, 15, 13,  3 }},
        {{ 11,  3, 17,  1,  9 }},
        {{  7, 15,  6, 18, 19 }},
        {{ 19,  5, 14,  4, 18 }},
        {{  5,  9,  1, 12, 14 }}
    }};

    std::array<face_type, 36> faces{};
    std::size_t               w = 0;

    for (std::size_t p = 0; p < 12; ++p)
    {
        const std::array<std::size_t, 5>& q = pents[p];

        // outward test from the first three vertices of the pentagon
        std::array<_T, 3> e1 = internal::platonic_sub(verts[q[1]], verts[q[0]]);
        std::array<_T, 3> e2 = internal::platonic_sub(verts[q[2]], verts[q[0]]);
        std::array<_T, 3> n  = internal::platonic_cross(e1, e2);

        std::array<_T, 3> centre = {{ z, z, z }};

        for (std::size_t v = 0; v < 5; ++v)
        {
            centre[0] += verts[q[v]][0];
            centre[1] += verts[q[v]][1];
            centre[2] += verts[q[v]][2];
        }

        std::array<std::size_t, 5> o;

        if (internal::platonic_dot(n, centre) < static_cast<_T>(0))
        {
            // reverse so the winding is outward
            o = {{ q[0], q[4], q[3], q[2], q[1] }};
        }
        else
        {
            o = {{ q[0], q[1], q[2], q[3], q[4] }};
        }

        // fan-triangulate the pentagon about its first vertex
        faces[w++] = face_type{{ o[0], o[1], o[2] }};
        faces[w++] = face_type{{ o[0], o[2], o[3] }};
        faces[w++] = face_type{{ o[0], o[3], o[4] }};
    }

    return mesh_type(verts, faces);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_PLATONIC_
