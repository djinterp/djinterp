/*******************************************************************************
* djinterp [cpp/math]                                               platonic.hpp
*
* The five Platonic solids as explicit triangle meshes.
*   Each factory returns a triangle_mesh<cartesian<3, T>, V, F> whose
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
*   tetrahedron_mesh<T>(radius)    -  4 vertices,  4 faces
*   hexahedron_mesh<T>(radius)     -  8 vertices, 12 faces   (the cube)
*   octahedron_mesh<T>(radius)     -  6 vertices,  8 faces
*   dodecahedron_mesh<T>(radius)   - 20 vertices, 36 faces   (12 pentagons)
*   icosahedron_mesh<T>(radius)    - 12 vertices, 20 faces
*
* LEVELS:
*   Everything compiles from C++98: a vertex is a linalg::vector, the
* vertex and face lists re_std::arrays, the winding test the vector's cross
* and dot products in the formulas' former order. A function template has
* no default type below C++11, so each factory also has a plain overload in
* double, for the call with no arguments, tetrahedron_mesh().
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/platonic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  INTERNAL ORIENTATION HELPERS
    ----------------------------
2.  TETRAHEDRON
    -----------
3.  HEXAHEDRON (CUBE)
    -----------------
4.  OCTAHEDRON
    ----------
5.  ICOSAHEDRON
    -----------
6.  DODECAHEDRON
    ------------
*/

#ifndef DJINTERP_MATH_GEOMETRY_PLATONIC_HPP
#define DJINTERP_MATH_GEOMETRY_PLATONIC_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../coordinate/coordinate.hpp"           // cartesian
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // the geometry's basics
#include "./solid.hpp"                            // triangle_mesh
// re_std
#include "../../../../../re_std/cpp/array/array.hpp"  // re_std::array


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  INTERNAL ORIENTATION HELPERS
//==============================================================================

NS_INTERNAL

    // outward_triangle
    //   function: the face (_i, _j, _k) wound so that its normal points away
    // from the origin -- (v_j - v_i) x (v_k - v_i), against the face's
    // vertex sum.
    template<typename    T,
             std::size_t V>
    re_std::array<std::size_t, 3>
    outward_triangle(
        const re_std::array<linalg::vector<T, 3>, V>& _verts,
        std::size_t                                   _i,
        std::size_t                                   _j,
        std::size_t                                   _k
    ) D_NOEXCEPT
    {
        const linalg::vector<T, 3> n = _verts[_j].minus(_verts[_i]).cross(
                                           _verts[_k].minus(_verts[_i]));
        const linalg::vector<T, 3> c = _verts[_i].plus(_verts[_j])
                                           .plus(_verts[_k]);
        re_std::array<std::size_t, 3> face = {{ _i, _j, _k }};

        // facing the origin: swap the last two
        if (n.dot(c) < static_cast<T>(0))
        {
            face[1] = _k;
            face[2] = _j;
        }

        return face;
    }

    // orient_triangles
    //   function: every face of _raw wound outward.
    template<typename    T,
             std::size_t V,
             std::size_t F>
    re_std::array<re_std::array<std::size_t, 3>, F>
    orient_triangles(
        const re_std::array<linalg::vector<T, 3>, V>&          _verts,
        const re_std::array<re_std::array<std::size_t, 3>, F>& _raw
    ) D_NOEXCEPT
    {
        re_std::array<re_std::array<std::size_t, 3>, F> out =
            re_std::array<re_std::array<std::size_t, 3>, F>();

        for (std::size_t f = 0; f < F; ++f)
        {
            out[f] = outward_triangle(_verts,
                                      _raw[f][0],
                                      _raw[f][1],
                                      _raw[f][2]);
        }

        return out;
    }

NS_END  // internal


//==============================================================================
// 2.  TETRAHEDRON
//==============================================================================

// tetrahedron_mesh
//   the regular tetrahedron inscribed in a sphere of radius _radius.
template<typename T>
triangle_mesh<cartesian<3, T>, 4, 4>
tetrahedron_mesh(T _radius = static_cast<T>(1))
{
    typedef triangle_mesh<cartesian<3, T>, 4, 4> mesh_type;
    typedef typename mesh_type::point_type point_type;
    typedef typename mesh_type::face_type face_type;

    // canonical circumradius is sqrt(3); scale onto the requested sphere
    T s = _radius * static_cast<T>(0.5773502691896258);

    re_std::array<point_type, 4> verts = {{
        point_type( s,  s,  s),
        point_type( s, -s, -s),
        point_type(-s,  s, -s),
        point_type(-s, -s,  s)
    }};

    re_std::array<face_type, 4> raw = {{
        {{ 0, 1, 2 }},
        {{ 0, 1, 3 }},
        {{ 0, 2, 3 }},
        {{ 1, 2, 3 }}
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}

// tetrahedron_mesh
//   the same in double, unit radius: the call with no arguments, which
// C++98 cannot give the template a default type for.
inline triangle_mesh<cartesian<3, double>, 4, 4>
tetrahedron_mesh()
{
    return tetrahedron_mesh<double>(1.0);
}


//==============================================================================
// 3.  HEXAHEDRON (CUBE)
//==============================================================================

// hexahedron_mesh
//   the cube inscribed in a sphere of radius _radius, each square face split
// into two triangles.
template<typename T>
triangle_mesh<cartesian<3, T>, 8, 12>
hexahedron_mesh(T _radius = static_cast<T>(1))
{
    typedef triangle_mesh<cartesian<3, T>, 8, 12> mesh_type;
    typedef typename mesh_type::point_type point_type;
    typedef typename mesh_type::face_type face_type;

    // canonical circumradius is sqrt(3)
    T s = _radius * static_cast<T>(0.5773502691896258);

    re_std::array<point_type, 8> verts = {{
        point_type(-s, -s, -s),   // 0
        point_type(s, -s, -s),   // 1
        point_type(s, s, -s),   // 2
        point_type(-s, s, -s),   // 3
        point_type(-s, -s, s),   // 4
        point_type(s, -s, s),   // 5
        point_type(s, s, s),   // 6
        point_type(-s, s, s)    // 7
    }};

    re_std::array<face_type, 12> raw = {{
        {{ 0, 1, 2 }}, {{ 0, 2, 3 }},   // z = -s
        {{ 4, 5, 6 }}, {{ 4, 6, 7 }},   // z = +s
        {{ 0, 1, 5 }}, {{ 0, 5, 4 }},   // y = -s
        {{ 3, 2, 6 }}, {{ 3, 6, 7 }},   // y = +s
        {{ 0, 3, 7 }}, {{ 0, 7, 4 }},   // x = -s
        {{ 1, 2, 6 }}, {{ 1, 6, 5 }}    // x = +s
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}

// hexahedron_mesh
//   the same in double, unit radius: the call with no arguments, which
// C++98 cannot give the template a default type for.
inline triangle_mesh<cartesian<3, double>, 8, 12>
hexahedron_mesh()
{
    return hexahedron_mesh<double>(1.0);
}


//==============================================================================
// 4.  OCTAHEDRON
//==============================================================================

// octahedron_mesh
//   the regular octahedron inscribed in a sphere of radius _radius (the
// vertices sit on the axes).
template<typename T>
triangle_mesh<cartesian<3, T>, 6, 8>
octahedron_mesh(T _radius = static_cast<T>(1))
{
    typedef triangle_mesh<cartesian<3, T>, 6, 8> mesh_type;
    typedef typename mesh_type::point_type point_type;
    typedef typename mesh_type::face_type face_type;

    // canonical circumradius is 1
    T s = _radius;
    T z = static_cast<T>(0);

    re_std::array<point_type, 6> verts = {{
        point_type(s, z, z),   // 0
        point_type(-s, z, z),   // 1
        point_type(z, s, z),   // 2
        point_type(z, -s, z),   // 3
        point_type(z, z, s),   // 4
        point_type(z, z, -s)    // 5
    }};

    re_std::array<face_type, 8> raw = {{
        {{ 0, 2, 4 }}, {{ 0, 2, 5 }}, {{ 0, 3, 4 }}, {{ 0, 3, 5 }},
        {{ 1, 2, 4 }}, {{ 1, 2, 5 }}, {{ 1, 3, 4 }}, {{ 1, 3, 5 }}
    }};

    return mesh_type(verts, internal::orient_triangles(verts, raw));
}

// octahedron_mesh
//   the same in double, unit radius: the call with no arguments, which
// C++98 cannot give the template a default type for.
inline triangle_mesh<cartesian<3, double>, 6, 8>
octahedron_mesh()
{
    return octahedron_mesh<double>(1.0);
}


//==============================================================================
// 5.  ICOSAHEDRON
//==============================================================================

// icosahedron_mesh
//   the regular icosahedron inscribed in a sphere of radius _radius.  The
// twelve vertices are the cyclic permutations of (0, +/-1, +/-phi).
template<typename T>
triangle_mesh<cartesian<3, T>, 12, 20>
icosahedron_mesh(T _radius = static_cast<T>(1))
{
    typedef triangle_mesh<cartesian<3, T>, 12, 20> mesh_type;
    typedef typename mesh_type::point_type point_type;
    typedef typename mesh_type::face_type face_type;

    T phi = static_cast<T>(1.6180339887498949);

    // canonical circumradius is sqrt(1 + phi^2)
    T s = _radius * static_cast<T>(0.5257311121191336);

    T a = s;
    T b = s * phi;
    T z = static_cast<T>(0);

    re_std::array<point_type, 12> verts = {{
        point_type(-a, b, z),   // 0
        point_type(a, b, z),   // 1
        point_type(-a, -b, z),   // 2
        point_type(a, -b, z),   // 3
        point_type(z, -a, b),   // 4
        point_type(z, a, b),   // 5
        point_type(z, -a, -b),   // 6
        point_type(z, a, -b),   // 7
        point_type(b, z, -a),   // 8
        point_type(b, z, a),   // 9
        point_type(-b, z, -a),   // 10
        point_type(-b, z, a)    // 11
    }};

    re_std::array<face_type, 20> raw = {{
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

// icosahedron_mesh
//   the same in double, unit radius: the call with no arguments, which
// C++98 cannot give the template a default type for.
inline triangle_mesh<cartesian<3, double>, 12, 20>
icosahedron_mesh()
{
    return icosahedron_mesh<double>(1.0);
}


//==============================================================================
// 6.  DODECAHEDRON
//==============================================================================

// dodecahedron_mesh
//   the regular dodecahedron inscribed in a sphere of radius _radius.  Its
// twenty vertices are the cube corners (+/-1, +/-1, +/-1) together with the
// cyclic permutations of (0, +/-1/phi, +/-phi).  Each of the twelve
// pentagonal faces is fan-triangulated into three triangles (36 total).
template<typename T>
triangle_mesh<cartesian<3, T>, 20, 36>
dodecahedron_mesh(T _radius = static_cast<T>(1))
{
    typedef triangle_mesh<cartesian<3, T>, 20, 36> mesh_type;
    typedef typename mesh_type::point_type point_type;
    typedef typename mesh_type::face_type face_type;

    T phi = static_cast<T>(1.6180339887498949);
    T inv = static_cast<T>(0.6180339887498949);

    // canonical circumradius is sqrt(3)
    T s  = _radius * static_cast<T>(0.5773502691896258);
    T c  = s;
    T pj = s * phi;
    T ij = s * inv;
    T z  = static_cast<T>(0);

    re_std::array<point_type, 20> verts = {{
        point_type(c, c, c),    // 0
        point_type(c, c, -c),    // 1
        point_type(c, -c, c),    // 2
        point_type(c, -c, -c),    // 3
        point_type(-c, c, c),    // 4
        point_type(-c, c, -c),    // 5
        point_type(-c, -c, c),    // 6
        point_type(-c, -c, -c),    // 7
        point_type(z, ij, pj),  // 8
        point_type(z, ij, -pj),  // 9
        point_type(z, -ij, pj),  // 10
        point_type(z, -ij, -pj),  // 11
        point_type(ij, pj, z),  // 12
        point_type(ij, -pj, z),  // 13
        point_type(-ij, pj, z),  // 14
        point_type(-ij, -pj, z),  // 15
        point_type(pj, z, ij),  // 16
        point_type(pj, z, -ij),  // 17
        point_type(-pj, z, ij),  // 18
        point_type(-pj, z, -ij)   // 19
    }};

    // the twelve pentagonal faces, in cycle order (winding fixed below)
    re_std::array<re_std::array<std::size_t, 5>, 12> pents = {{
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

    re_std::array<face_type, 36> faces = re_std::array<face_type, 36>();
    std::size_t                  w     = 0;

    for (std::size_t p = 0; p < 12; ++p)
    {
        const re_std::array<std::size_t, 5>& q = pents[p];

        // outward test from the first three vertices of the pentagon
        const point_type n = verts[q[1]].minus(verts[q[0]])
                                 .cross(verts[q[2]].minus(verts[q[0]]));
        point_type       centre(z, z, z);

        for (std::size_t v = 0; v < 5; ++v)
        {
            centre[0] += verts[q[v]][0];
            centre[1] += verts[q[v]][1];
            centre[2] += verts[q[v]][2];
        }

        re_std::array<std::size_t, 5> o = q;

        // facing the origin: reverse so the winding is outward
        if (n.dot(centre) < static_cast<T>(0))
        {
            o[1] = q[4];
            o[2] = q[3];
            o[3] = q[2];
            o[4] = q[1];
        }

        // fan-triangulate the pentagon about its first vertex
        for (std::size_t t = 0; t < 3; ++t)
        {
            faces[w][0] = o[0];
            faces[w][1] = o[t + 1];
            faces[w][2] = o[t + 2];
            ++w;
        }
    }

    return mesh_type(verts, faces);
}

// dodecahedron_mesh
//   the same in double, unit radius: the call with no arguments, which
// C++98 cannot give the template a default type for.
inline triangle_mesh<cartesian<3, double>, 20, 36>
dodecahedron_mesh()
{
    return dodecahedron_mesh<double>(1.0);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_PLATONIC_HPP
