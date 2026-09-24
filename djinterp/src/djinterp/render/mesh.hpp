/*******************************************************************************
* djinterp [3d]                                                         mesh.hpp
*
*   A runtime, attribute-carrying render mesh: a flat vertex-position buffer,
* optional per-vertex normals, and a triangle-list index buffer. This is the
* drawable counterpart to the geometry subframework's fixed-size, analytic
* triangle_mesh; it adds the things a renderer needs (smooth normals, the
* unique wireframe edge set, transform application) and grows at runtime.
*
*   make_mesh() adapts any analytic mesh that exposes m_vertices / m_faces
* (e.g. math::triangle_mesh, including the Platonic solids) into a render mesh.
* It is a template over that structure, so this header does not depend on the
* geometry subframework -- only on linalg for positions and transforms.
*
* path:      /inc/djinterp/3d/mesh.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_3D_MESH_HPP
#define DJINTERP_3D_MESH_HPP 1

// std
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <set>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../math/linear_algebra/vector.hpp"
#include "../math/linear_algebra/matrix.hpp"
#include "../math/linear_algebra/transform.hpp"   // transform_point, transform_direction


NS_DJINTERP
namespace render3d
{

// mesh
//   struct: a triangle mesh held in runtime buffers. m_indices is a triangle
// list (three indices per face) into m_positions; m_normals is parallel to
// m_positions when present (it may be empty until compute_normals() runs).
template<typename _T = double>
struct mesh
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _T;
    using vec3_type  = math::linalg::vector<_T, 3>;
    using matrix_type = math::linalg::matrix<_T, 4, 4>;
    using index_type = std::uint32_t;
    using edge_type  = std::array<index_type, 2>;

    // ---- data ---------------------------------------------------------------

    std::vector<vec3_type>  m_positions;
    std::vector<vec3_type>  m_normals;
    std::vector<index_type> m_indices;

    // ---- construction -------------------------------------------------------

    mesh() noexcept
        : m_positions(),
          m_normals(),
          m_indices()
    {}

    mesh(
        const std::vector<vec3_type>&  _positions,
        const std::vector<index_type>& _indices
    )
        : m_positions(_positions),
          m_normals(),
          m_indices(_indices)
    {}

    // ---- queries ------------------------------------------------------------

    // vertex_count
    //   the number of vertices.
    D_NODISCARD std::size_t
    vertex_count() const noexcept
    {
        return m_positions.size();
    }

    // triangle_count
    //   the number of triangles (index count / 3).
    D_NODISCARD std::size_t
    triangle_count() const noexcept
    {
        return m_indices.size() / 3;
    }

    // has_normals
    //   true when a normal is stored per vertex.
    D_NODISCARD bool
    has_normals() const noexcept
    {
        return ( !m_normals.empty() &&
                 (m_normals.size() == m_positions.size()) );
    }

    // ---- mutation -----------------------------------------------------------

    // compute_normals
    //   (re)computes smooth per-vertex normals by accumulating area-weighted
    // face normals into each vertex and normalising.
    void
    compute_normals() noexcept
    {
        m_normals.assign(m_positions.size(), vec3_type());

        const std::size_t tris = triangle_count();

        for (std::size_t f = 0; f < tris; ++f)
        {
            const index_type i0 = m_indices[3 * f + 0];
            const index_type i1 = m_indices[3 * f + 1];
            const index_type i2 = m_indices[3 * f + 2];

            // area-weighted (un-normalised) face normal
            const vec3_type fn = cross(m_positions[i1] - m_positions[i0],
                                       m_positions[i2] - m_positions[i0]);

            m_normals[i0] = m_normals[i0] + fn;
            m_normals[i1] = m_normals[i1] + fn;
            m_normals[i2] = m_normals[i2] + fn;
        }

        for (std::size_t i = 0; i < m_normals.size(); ++i)
        {
            m_normals[i] = normalize(m_normals[i]);
        }

        return;
    }

    // transform
    //   applies the affine transform _m to every position, and to every normal
    // via the linear part. The linear part is correct for rigid and
    // uniform-scale transforms; a non-uniform scale would require the
    // inverse-transpose, which is intentionally not done here.
    void
    transform(
        const matrix_type& _m
    ) noexcept
    {
        for (std::size_t i = 0; i < m_positions.size(); ++i)
        {
            m_positions[i] = transform_point(_m, m_positions[i]);
        }

        for (std::size_t i = 0; i < m_normals.size(); ++i)
        {
            m_normals[i] = normalize(transform_direction(_m, m_normals[i]));
        }

        return;
    }

    // edges
    //   the unique undirected edge set of the mesh, each edge reported once as
    // an ordered index pair. Suitable for wireframe rendering.
    D_NODISCARD std::vector<edge_type>
    edges() const
    {
        std::set<std::pair<index_type, index_type>> seen;
        std::vector<edge_type>                      result;

        const std::size_t tris = triangle_count();

        for (std::size_t f = 0; f < tris; ++f)
        {
            index_type tri[3];

            tri[0] = m_indices[3 * f + 0];
            tri[1] = m_indices[3 * f + 1];
            tri[2] = m_indices[3 * f + 2];

            for (std::size_t e = 0; e < 3; ++e)
            {
                index_type a = tri[e];
                index_type b = tri[(e + 1) % 3];

                // canonical orientation so (a, b) and (b, a) coincide
                if (a > b)
                {
                    const index_type tmp = a;
                    a = b;
                    b = tmp;
                }

                if (seen.insert(std::make_pair(a, b)).second)
                {
                    result.push_back(edge_type{{ a, b }});
                }
            }
        }

        return result;
    }
};

// make_mesh
//   builds a render mesh from any analytic mesh exposing m_vertices (a
// container of point_type, i.e. std::array<value_type, 3>) and m_faces (a
// container of triples of vertex indices), then computes smooth normals.
// Works on math::triangle_mesh and the Platonic-solid factories without this
// header depending on the geometry subframework.
template<typename _TriMesh>
D_NODISCARD mesh<typename _TriMesh::value_type>
make_mesh(
    const _TriMesh& _tri
)
{
    using _T = typename _TriMesh::value_type;

    mesh<_T> out;

    out.m_positions.reserve(_tri.m_vertices.size());

    for (const auto& v : _tri.m_vertices)
    {
        out.m_positions.push_back(math::linalg::vector<_T, 3>(v));
    }

    out.m_indices.reserve(_tri.m_faces.size() * 3);

    for (const auto& f : _tri.m_faces)
    {
        out.m_indices.push_back(static_cast<std::uint32_t>(f[0]));
        out.m_indices.push_back(static_cast<std::uint32_t>(f[1]));
        out.m_indices.push_back(static_cast<std::uint32_t>(f[2]));
    }

    out.compute_normals();

    return out;
}

}  // namespace render3d
NS_END  // djinterp


#endif  // DJINTERP_3D_MESH_HPP
