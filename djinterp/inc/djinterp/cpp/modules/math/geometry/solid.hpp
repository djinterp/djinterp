/*******************************************************************************
* djinterp [cpp/math]                                                  solid.hpp
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
* LEVELS:
*   triangle_mesh and parametric_volume compile from C++98: points are
* linalg::vectors, a mesh's vertices and faces re_std::arrays (from C++11 a
* mesh is built from std::arrays too), and containment casts a ray through
* the boundary with the C core's Moller-Trumbore (c/math/isect.h, through
* ray.hpp), the one ray.hpp's intersect_triangle uses. implicit_volume
* evaluates on a std::tuple and shape_from_surfaces is variadic: both are
* C++11's.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/solid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRIANGLE MESHES
    ---------------
    1.  triangle_mesh
2.  PARAMETRIC VOLUMES
    ------------------
    1.  parametric_volume
3.  IMPLICIT VOLUMES
    ----------------
    1.  implicit_volume
4.  SHAPES FROM SURFACES
    --------------------
    1.  shape_from_surfaces
*/

#ifndef DJINTERP_MATH_GEOMETRY_SOLID_HPP
#define DJINTERP_MATH_GEOMETRY_SOLID_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // is_surface
#include "./ray.hpp"                              // internal::ray_isect
#include "./surface.hpp"                          // the surfaces
// re_std
#include "../../../../../re_std/cpp/array/array.hpp"  // re_std::array
// std and re_std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                              // std::array
    #include <tuple>                              // std::tuple, std::get
    #include "../../../../../re_std/cpp/utility/integer_sequence.hpp"       // index_
                                                                            // sequence
    #include "../../../../../re_std/cpp/utility/make_integer_sequence.hpp"  // make_
                                                                            // index_
                                                                            // sequence
#endif


NS_DJINTERP
NS_MATH


NS_INTERNAL

    // solid_ray_hits
    //   function: whether the +x ray from _origin crosses the triangle (_v0,
    // _v1, _v2) ahead of it (Moller-Trumbore, epsilon 1e-9): the boundary
    // crossing a containment count adds up.
    template<typename T>
    D_CONSTEXPR_CPP14 bool
    solid_ray_hits(
        const linalg::vector<T, 3>& _origin,
        const linalg::vector<T, 3>& _v0,
        const linalg::vector<T, 3>& _v1,
        const linalg::vector<T, 3>& _v2
    ) D_NOEXCEPT
    {
        const T                    dir_e[3] = { static_cast<T>(1),
                                                static_cast<T>(0),
                                                static_cast<T>(0) };
        const linalg::vector<T, 3> dir(dir_e);
        T                          t = T();

        return ray_isect<T>::triangle(_origin.data(),
                                      dir.data(),
                                      _v0.data(),
                                      _v1.data(),
                                      _v2.data(),
                                      static_cast<T>(1e-9),
                                      &t);
    }

    // solid_surface_hits
    //   function: the boundary crossings of the +x ray from _origin through
    // _surf, sampled as Samples x Samples quads, each split into two
    // triangles (p00 p10 p11, p00 p11 p01).
    template<std::size_t Samples,
             typename    System,
             typename    Surface>
    std::size_t
    solid_surface_hits(
        const Surface&                     _surf,
        const typename System::point_type& _origin
    )
    {
        typedef typename System::value_type value_type;
        typedef typename System::point_type point_type;

        const value_type u_min = _surf.parameter_min_u();
        const value_type u_max = _surf.parameter_max_u();
        const value_type v_min = _surf.parameter_min_v();
        const value_type v_max = _surf.parameter_max_v();
        const value_type du    = (u_max - u_min) /
                                 static_cast<value_type>(Samples);
        const value_type dv    = (v_max - v_min) /
                                 static_cast<value_type>(Samples);
        std::size_t      hits  = 0;

        for (std::size_t i = 0; i < Samples; ++i)
        {
            for (std::size_t j = 0; j < Samples; ++j)
            {
                const value_type u0  = u_min +
                                       (static_cast<value_type>(i) * du);
                const value_type u1  = u0 + du;
                const value_type v0  = v_min +
                                       (static_cast<value_type>(j) * dv);
                const value_type v1  = v0 + dv;
                const point_type p00 = System::to_cartesian(_surf.at(u0, v0));
                const point_type p10 = System::to_cartesian(_surf.at(u1, v0));
                const point_type p01 = System::to_cartesian(_surf.at(u0, v1));
                const point_type p11 = System::to_cartesian(_surf.at(u1, v1));

                // the quad's two triangles
                hits += solid_ray_hits(_origin, p00, p10, p11) ? 1 : 0;
                hits += solid_ray_hits(_origin, p00, p11, p01) ? 1 : 0;
            }
        }

        return hits;
    }

NS_END  // internal


//==============================================================================
// 1.  TRIANGLE MESHES
//==============================================================================


// 1.1    triangle_mesh
//------------------------------------------------------------------------------
// 1.1.1
// triangle_mesh
//   struct: a closed solid bounded by F triangles over V vertices, each face
// three vertex indices wound outward.
template<typename    System,
         std::size_t V,
         std::size_t F>
struct triangle_mesh
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "triangle_mesh: 3D coordinate systems only.");
    D_STATIC_ASSERT((V >= 4),
                    "triangle_mesh: at least 4 vertices for a closed solid.");
    D_STATIC_ASSERT((F >= 4),
                    "triangle_mesh: at least 4 faces for a closed solid.");

    typedef System                         coordinate_system;
    typedef typename System::value_type    value_type;
    typedef typename System::point_type    point_type;
    typedef re_std::array<std::size_t, 3>  face_type;
    typedef re_std::array<point_type, V>   vertex_array;
    typedef re_std::array<face_type, F>    face_array;

    static D_CONSTEXPR_VAR std::size_t dimension    = 3;
    static D_CONSTEXPR_VAR std::size_t vertex_count = V;
    static D_CONSTEXPR_VAR std::size_t face_count   = F;
    static D_CONSTEXPR_VAR bool        is_shape     = true;
    static D_CONSTEXPR_VAR bool        is_2d        = false;
    static D_CONSTEXPR_VAR bool        is_3d        = true;

    vertex_array m_vertices;
    face_array   m_faces;

    D_CONSTEXPR
    triangle_mesh() D_NOEXCEPT
        : m_vertices(),
          m_faces()
    {}

    D_CONSTEXPR
    triangle_mesh(
        const vertex_array& _verts,
        const face_array&   _faces
    ) D_NOEXCEPT
        : m_vertices(_verts),
          m_faces(_faces)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from std::arrays of vertices and faces (C++11), as the mesh held them
    template<typename Point,
             typename Face>
    D_CONSTEXPR_CPP14
    triangle_mesh(
        const std::array<Point, V>& _verts,
        const std::array<Face, F>&  _faces
    ) D_NOEXCEPT
        : m_vertices(),
          m_faces()
    {
        for (std::size_t i = 0; i < V; ++i)
        {
            m_vertices[i] = point_type(_verts[i]);
        }

        for (std::size_t i = 0; i < F; ++i)
        {
            for (std::size_t k = 0; k < 3; ++k)
            {
                m_faces[i][k] = static_cast<std::size_t>(_faces[i][k]);
            }
        }
    }
#endif

    // contains
    //   whether _p lies inside: the +x ray from it crosses the boundary an
    // odd number of times.
    D_CONSTEXPR_CPP14 bool
    contains(const point_type& _p) const D_NOEXCEPT
    {
        const point_type p    = System::to_cartesian(_p);
        std::size_t      hits = 0;

        for (std::size_t i = 0; i < F; ++i)
        {
            hits += internal::solid_ray_hits(
                        p,
                        System::to_cartesian(m_vertices[m_faces[i][0]]),
                        System::to_cartesian(m_vertices[m_faces[i][1]]),
                        System::to_cartesian(m_vertices[m_faces[i][2]]))
                        ? 1 : 0;
        }

        return ((hits % 2) == 1);
    }

    // closed_form_surface_area
    //   the sum of the faces' areas, |(v1 - v0) x (v2 - v0)| / 2 each.
    D_CONSTEXPR_CPP14 value_type
    closed_form_surface_area() const D_NOEXCEPT
    {
        value_type total = static_cast<value_type>(0);

        for (std::size_t i = 0; i < F; ++i)
        {
            const point_type v0 =
                System::to_cartesian(m_vertices[m_faces[i][0]]);
            const point_type v1 =
                System::to_cartesian(m_vertices[m_faces[i][1]]);
            const point_type v2 =
                System::to_cartesian(m_vertices[m_faces[i][2]]);

            total += v1.minus(v0).cross(v2.minus(v0)).norm() /
                     static_cast<value_type>(2);
        }

        return total;
    }

    // closed_form_volume
    //   |Σ v0 · (v1 x v2)| / 6, by the divergence theorem.
    D_CONSTEXPR_CPP14 value_type
    closed_form_volume() const D_NOEXCEPT
    {
        value_type sum = static_cast<value_type>(0);

        for (std::size_t i = 0; i < F; ++i)
        {
            const point_type v0 =
                System::to_cartesian(m_vertices[m_faces[i][0]]);
            const point_type v1 =
                System::to_cartesian(m_vertices[m_faces[i][1]]);
            const point_type v2 =
                System::to_cartesian(m_vertices[m_faces[i][2]]);

            sum += v0.dot(v1.cross(v2));
        }

        const value_type result = sum / static_cast<value_type>(6);

        return (result < static_cast<value_type>(0)) ? -result : result;
    }

    // closed_form_centroid
    //   the volume-weighted centre of the faces' tetrahedra from the origin;
    // the first vertex for a mesh of no volume.
    D_CONSTEXPR_CPP14 point_type
    closed_form_centroid() const D_NOEXCEPT
    {
        point_type cx_acc;
        value_type volume_sum = static_cast<value_type>(0);

        for (std::size_t i = 0; i < F; ++i)
        {
            const point_type v0 =
                System::to_cartesian(m_vertices[m_faces[i][0]]);
            const point_type v1 =
                System::to_cartesian(m_vertices[m_faces[i][1]]);
            const point_type v2 =
                System::to_cartesian(m_vertices[m_faces[i][2]]);
            const value_type vol6 = v0.dot(v1.cross(v2));

            // vol6 (v0 + v1 + v2) / 4, accumulated
            cx_acc = cx_acc.plus(v0.plus(v1).plus(v2).scaled(vol6)
                                     .divided(static_cast<value_type>(4)));
            volume_sum += vol6;
        }

        // no volume: no centre to weigh
        if (volume_sum == static_cast<value_type>(0))
        {
            return m_vertices[0];
        }

        return System::from_cartesian(cx_acc.divided(volume_sum));
    }
};

template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR std::size_t triangle_mesh<System, V, F>::dimension;
template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR std::size_t triangle_mesh<System, V, F>::vertex_count;
template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR std::size_t triangle_mesh<System, V, F>::face_count;
template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR bool triangle_mesh<System, V, F>::is_shape;
template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR bool triangle_mesh<System, V, F>::is_2d;
template<typename System, std::size_t V, std::size_t F>
D_CONSTEXPR_VAR bool triangle_mesh<System, V, F>::is_3d;


//==============================================================================
// 2.  PARAMETRIC VOLUMES
//==============================================================================


// 2.1    parametric_volume
//------------------------------------------------------------------------------
// 2.1.1
// parametric_volume
//   struct: the solid a closed parametric surface bounds.
template<typename Surface>
struct parametric_volume
{
    D_STATIC_ASSERT((is_surface<Surface>::value),
                    "parametric_volume: Surface must satisfy the surface "
                    "interface.");
    D_STATIC_ASSERT((Surface::dimension == 3),
                    "parametric_volume: 3D surfaces only.");

    typedef Surface                              boundary_type;
    typedef typename Surface::coordinate_system  coordinate_system;
    typedef typename Surface::value_type         value_type;
    typedef typename Surface::point_type         point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = 3;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = false;
    static D_CONSTEXPR_VAR bool        is_3d     = true;

    boundary_type m_boundary;

    D_CONSTEXPR
    parametric_volume() D_NOEXCEPT
        : m_boundary()
    {}

    D_CONSTEXPR explicit
    parametric_volume(const boundary_type& _b) D_NOEXCEPT
        : m_boundary(_b)
    {}

    const boundary_type&
    boundary() const D_NOEXCEPT
    {
        return m_boundary;
    }

    // contains
    //   whether _p lies inside: the +x ray's crossings of the boundary,
    // sampled as Samples x Samples quads, are odd (Samples 32 by default;
    // contains<N>(p) for another).
    template<std::size_t Samples>
    bool
    contains(const point_type& _p) const
    {
        return ((internal::solid_surface_hits<Samples, coordinate_system>(
                     m_boundary, coordinate_system::to_cartesian(_p)) % 2)
                == 1);
    }

    bool
    contains(const point_type& _p) const
    {
        return contains<32>(_p);
    }
};

template<typename Surface>
D_CONSTEXPR_VAR std::size_t parametric_volume<Surface>::dimension;
template<typename Surface>
D_CONSTEXPR_VAR bool parametric_volume<Surface>::is_shape;
template<typename Surface>
D_CONSTEXPR_VAR bool parametric_volume<Surface>::is_2d;
template<typename Surface>
D_CONSTEXPR_VAR bool parametric_volume<Surface>::is_3d;


//==============================================================================
// 3.  IMPLICIT VOLUMES
//==============================================================================
// From C++11: the expression evaluates on a std::tuple.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// 3.1    implicit_volume
//------------------------------------------------------------------------------
// 3.1.1
// implicit_volume
//   struct: the solid F(x, y, z) <= 0, F a type with a static evaluate(std::
// tuple of x, y, z).
template<typename Expr,
         typename System = cartesian<3, double> >
struct implicit_volume
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "implicit_volume: 3D coordinate systems only.");

    typedef System                      coordinate_system;
    typedef Expr                        expression_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static constexpr std::size_t dimension   = 3;
    static constexpr bool        is_shape    = true;
    static constexpr bool        is_2d       = false;
    static constexpr bool        is_3d       = true;
    static constexpr bool        is_implicit = true;

    // contains
    //   F(p) <= 0, on p's Cartesian form.
    bool
    contains(const point_type& _p) const
    {
        const point_type cart = System::to_cartesian(_p);

        return ( Expr::evaluate(std::make_tuple(cart[0], cart[1], cart[2])) <=
                 static_cast<value_type>(0) );
    }

    // on_boundary
    //   |F(p)| < _epsilon.
    bool
    on_boundary(
        const point_type& _p,
        value_type        _epsilon = static_cast<value_type>(1e-9)
    ) const
    {
        const point_type cart = System::to_cartesian(_p);
        value_type       v    = Expr::evaluate(
            std::make_tuple(cart[0], cart[1], cart[2]));

        if (v < static_cast<value_type>(0))
        {
            v = -v;
        }

        return (v < _epsilon);
    }
};

template<typename Expr, typename System>
constexpr std::size_t implicit_volume<Expr, System>::dimension;
template<typename Expr, typename System>
constexpr bool implicit_volume<Expr, System>::is_shape;
template<typename Expr, typename System>
constexpr bool implicit_volume<Expr, System>::is_2d;
template<typename Expr, typename System>
constexpr bool implicit_volume<Expr, System>::is_3d;
template<typename Expr, typename System>
constexpr bool implicit_volume<Expr, System>::is_implicit;


//==============================================================================
// 4.  SHAPES FROM SURFACES
//==============================================================================
// From C++11: variadic.


// 4.1    shape_from_surfaces
//------------------------------------------------------------------------------
// 4.1.1
// shape_from_surfaces
//   struct: the solid an ordered tuple of surfaces bounds together.
template<typename... Surfaces>
struct shape_from_surfaces
{
    static_assert(sizeof...(Surfaces) >= 1,
                  "shape_from_surfaces: at least one surface required.");

    typedef typename std::tuple_element<0, std::tuple<Surfaces...> >::type
        first_surface_type;
    typedef typename first_surface_type::coordinate_system coordinate_system;
    typedef typename coordinate_system::value_type         value_type;
    typedef typename coordinate_system::point_type         point_type;
    typedef std::tuple<Surfaces...>                        surfaces_type;

    static constexpr std::size_t dimension     = coordinate_system::dimension;
    static constexpr std::size_t surface_count = sizeof...(Surfaces);
    static constexpr bool        is_shape      = true;
    static constexpr bool        is_2d         = false;
    static constexpr bool        is_3d         = true;

    surfaces_type m_surfaces;

    constexpr
    shape_from_surfaces() noexcept
        : m_surfaces()
    {}

    constexpr explicit
    shape_from_surfaces(const Surfaces&... _s) noexcept
        : m_surfaces(_s...)
    {}

    const surfaces_type&
    surfaces() const noexcept
    {
        return m_surfaces;
    }

    // contains
    //   whether _p lies inside: the +x ray's crossings of every surface,
    // each sampled as SamplesPerSurface squared quads, are odd in sum.
    template<std::size_t SamplesPerSurface>
    bool
    contains(const point_type& _p) const
    {
        return ((count_hits<SamplesPerSurface>(
                     coordinate_system::to_cartesian(_p),
                     re_std::make_index_sequence<sizeof...(Surfaces)>()) %
                 2) == 1);
    }

    bool
    contains(const point_type& _p) const
    {
        return contains<16>(_p);
    }

private:
    // count_hits
    //   the crossings of each surface, in order, summed.
    template<std::size_t SPS,
             std::size_t... Is>
    std::size_t
    count_hits(
        const point_type&             _origin,
        re_std::index_sequence<Is...>
    ) const
    {
        const std::size_t counts[] = {
            internal::solid_surface_hits<SPS, coordinate_system>(
                std::get<Is>(m_surfaces), _origin)...
        };
        std::size_t       total    = 0;

        for (std::size_t i = 0; i < sizeof...(Surfaces); ++i)
        {
            total += counts[i];
        }

        return total;
    }
};

template<typename... Surfaces>
constexpr std::size_t shape_from_surfaces<Surfaces...>::dimension;
template<typename... Surfaces>
constexpr std::size_t shape_from_surfaces<Surfaces...>::surface_count;
template<typename... Surfaces>
constexpr bool shape_from_surfaces<Surfaces...>::is_shape;
template<typename... Surfaces>
constexpr bool shape_from_surfaces<Surfaces...>::is_2d;
template<typename... Surfaces>
constexpr bool shape_from_surfaces<Surfaces...>::is_3d;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SOLID_HPP
