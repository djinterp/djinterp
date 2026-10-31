/*******************************************************************************
* djinterp [cpp/math]                                               shape_2d.hpp
*
* General-purpose 2D shape composers.
*   Provides the four ways a user can describe an arbitrary planar
* shape: by vertex list (polygon), by closed parametric curve
* (parametric_region), by implicit inequality (implicit_region), or by
* an ordered tuple of heterogeneous edges (shape_from_edges). Named
* shapes (circle, rectangle, etc.) live in named_2d.hpp.
*
* Each shape satisfies the structural shape interface from
* geometry_common.hpp; measurement (area, perimeter, centroid) is
* handled separately in measure_2d.hpp.
*
* LEVELS:
*   polygon and parametric_region compile from C++98: points are
* linalg::vectors, a polygon's vertices a re_std::array (from C++11 a
* std::array of points converts), and the crossing test, the shoelace
* area and the centroid are c/math/plane.h's -- for float, double and long
* double, and through double for any other element type. implicit_region
* evaluates on a std::tuple and shape_from_edges is variadic: both are
* C++11's.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/shape_2d.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  POLYGONS
    --------
    1.  polygon
2.  PARAMETRIC REGIONS
    ------------------
    1.  parametric_region
3.  IMPLICIT REGIONS
    ----------------
    1.  implicit_region
4.  SHAPES FROM EDGES
    -----------------
    1.  shape_from_edges
*/

#ifndef DJINTERP_MATH_GEOMETRY_SHAPE_2D_HPP
#define DJINTERP_MATH_GEOMETRY_SHAPE_2D_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../../../../c/modules/math/plane.h"                   // the C core: d_plane_*
#if (defined(__cplusplus))
    #include "../../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/linalg_common.hpp"    // linalg::internal::
                                                  // linalg_family_of
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./edge.hpp"                             // the edges
#include "./geometry_common.hpp"                  // cartesian_distance,
                                                  // is_edge
// re_std
#include "../../../../../re_std/cpp/array/array.hpp"        // re_std::array
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

    // plane_kernel
    //   struct: the C core's planar kernels for float, double and long
    // double, and the same through double for any other element type.
    template<typename T,
             int      Family = linalg::internal::linalg_family_of<T>::value>
    struct plane_kernel
    {
        static D_CONSTEXPR_CPP14 bool
        ray_crosses(const T* _p,
                    const T* _a,
                    const T* _b) D_NOEXCEPT
        {
            return d_plane_ray_crosses(_p, _a, _b);
        }

        static D_CONSTEXPR_CPP14 bool
        polygon_contains(const T*    _xy,
                         std::size_t _n,
                         const T*    _p) D_NOEXCEPT
        {
            return d_plane_polygon_contains(_xy, _n, _p);
        }

        static D_CONSTEXPR_CPP14 T
        polygon_area(const T*    _xy,
                     std::size_t _n) D_NOEXCEPT
        {
            return d_plane_polygon_area(_xy, _n);
        }

        static D_CONSTEXPR_CPP14 bool
        polygon_centroid(T*          _out,
                         const T*    _xy,
                         std::size_t _n) D_NOEXCEPT
        {
            return d_plane_polygon_centroid(_out, _xy, _n);
        }
    };

    template<typename T>
    struct plane_kernel<T, 0>
    {
        static D_CONSTEXPR_CPP14
        bool ray_crosses(
            const T* _p,
            const T* _a,
            const T* _b
        ) D_NOEXCEPT
        {
            const double p[2] = { static_cast<double>(_p[0]),
                                  static_cast<double>(_p[1]) };
            const double a[2] = { static_cast<double>(_a[0]),
                                  static_cast<double>(_a[1]) };
            const double b[2] = { static_cast<double>(_b[0]),
                                  static_cast<double>(_b[1]) };

            return d_plane_d_ray_crosses(p, a, b);
        }

        // the polygon kernels take the vertices through double, n at most
        // the shape's own count, which the face passes
        template<std::size_t N>
        static D_CONSTEXPR_CPP14
        bool polygon_contains_n(
            const T* _xy,
            const T* _p
        ) D_NOEXCEPT
        {
            double       xy[2 * N] = {};
            const double p[2]      = { static_cast<double>(_p[0]),
                                       static_cast<double>(_p[1]) };

            for (std::size_t i = 0; i < (2 * N); ++i)
            {
                xy[i] = static_cast<double>(_xy[i]);
            }

            return d_plane_d_polygon_contains(xy, N, p);
        }

        template<std::size_t N>
        static D_CONSTEXPR_CPP14
        T polygon_area_n(
            const T* _xy
        ) D_NOEXCEPT
        {
            double xy[2 * N] = {};

            for (std::size_t i = 0; i < (2 * N); ++i)
            {
                xy[i] = static_cast<double>(_xy[i]);
            }

            return static_cast<T>(d_plane_d_polygon_area(xy, N));
        }

        template<std::size_t N>
        static D_CONSTEXPR_CPP14
        bool polygon_centroid_n(
            T*       _out,
            const T* _xy
        ) D_NOEXCEPT
        {
            double xy[2 * N] = {};
            double c[2]      = { 0.0, 0.0 };

            for (std::size_t i = 0; i < (2 * N); ++i)
            {
                xy[i] = static_cast<double>(_xy[i]);
            }

            const bool found = d_plane_d_polygon_centroid(c, xy, N);

            // the centre only where there is one, as the families' kernel
            if (found)
            {
                _out[0] = static_cast<T>(c[0]);
                _out[1] = static_cast<T>(c[1]);
            }

            return found;
        }
    };

    // plane_polygon
    //   struct: the polygon kernels for N vertices, whichever the family.
    template<typename    T,
             std::size_t N,
             int         Family = linalg::internal::linalg_family_of<T>::value>
    struct plane_polygon
    {
        static D_CONSTEXPR_CPP14 bool
        contains(const T* _xy,
                 const T* _p) D_NOEXCEPT
        {
            return plane_kernel<T>::polygon_contains(_xy, N, _p);
        }

        static D_CONSTEXPR_CPP14 T
        area(const T* _xy) D_NOEXCEPT
        {
            return plane_kernel<T>::polygon_area(_xy, N);
        }

        static D_CONSTEXPR_CPP14 bool
        centroid(T*       _out,
                 const T* _xy) D_NOEXCEPT
        {
            return plane_kernel<T>::polygon_centroid(_out, _xy, N);
        }
    };

    template<typename    T,
             std::size_t N>
    struct plane_polygon<T, N, 0>
    {
        static D_CONSTEXPR_CPP14
        bool contains(
            const T* _xy,
            const T* _p
        ) D_NOEXCEPT
        {
            return plane_kernel<T>::template polygon_contains_n<N>(_xy, _p);
        }

        static D_CONSTEXPR_CPP14
        T area(
            const T* _xy
        ) D_NOEXCEPT
        {
            return plane_kernel<T>::template polygon_area_n<N>(_xy);
        }

        static D_CONSTEXPR_CPP14
        bool centroid(
            T*       _out,
            const T* _xy
        ) D_NOEXCEPT
        {
            return plane_kernel<T>::template polygon_centroid_n<N>(_out, _xy);
        }
    };

    // plane_edge_crossings
    //   function: the +x ray's crossings of _edge, sampled as Samples
    // straight pieces in its parameter.
    template<std::size_t Samples,
             typename    System,
             typename    Edge>
    std::size_t plane_edge_crossings(
        const Edge&                        _edge,
        const typename System::point_type& _cart_p
    )
    {
        typedef typename System::value_type value_type;
        typedef typename System::point_type point_type;

        const value_type t_min     = _edge.parameter_min();
        const value_type t_max     = _edge.parameter_max();
        const value_type step      = (t_max - t_min) /
                                     static_cast<value_type>(Samples);
        std::size_t      crossings = 0;

        for (std::size_t k = 0; k < Samples; ++k)
        {
            const value_type t1 = t_min + (static_cast<value_type>(k) * step);
            const value_type t2 = t_min +
                                  (static_cast<value_type>(k + 1) * step);
            const point_type a  = System::to_cartesian(_edge.at(t1));
            const point_type b  = System::to_cartesian(_edge.at(t2));

            // one more crossing where the ray meets this piece
            if (plane_kernel<value_type>::ray_crosses(_cart_p.data(),
                                                      a.data(),
                                                      b.data()))
            {
                ++crossings;
            }
        }

        return crossings;
    }

NS_END  // internal


//==============================================================================
// 1.  POLYGONS
//==============================================================================


// 1.1    polygon
//------------------------------------------------------------------------------
// 1.1.1
// polygon
//   struct: a simple polygon of N vertices in a 2D system.
template<typename    System,
         std::size_t N>
struct polygon
{
    D_STATIC_ASSERT((System::dimension == 2),
                    "polygon: requires a 2D coordinate system.");
    D_STATIC_ASSERT((N >= 3), "polygon: at least 3 vertices required.");

    typedef System                       coordinate_system;
    typedef typename System::value_type  value_type;
    typedef typename System::point_type  point_type;
    typedef re_std::array<point_type, N> vertex_array;

    static D_CONSTEXPR_VAR std::size_t dimension    = 2;
    static D_CONSTEXPR_VAR std::size_t vertex_count = N;
    static D_CONSTEXPR_VAR bool        is_shape     = true;
    static D_CONSTEXPR_VAR bool        is_2d        = true;
    static D_CONSTEXPR_VAR bool        is_3d        = false;

    vertex_array m_vertices;

    D_CONSTEXPR
    polygon() D_NOEXCEPT
        : m_vertices()
    {}

    D_CONSTEXPR explicit
    polygon(const vertex_array& _verts) D_NOEXCEPT
        : m_vertices(_verts)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from a std::array of points (C++11), as the vertices were held
    D_CONSTEXPR_CPP14 explicit
    polygon(const std::array<point_type, N>& _verts) D_NOEXCEPT
        : m_vertices()
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            m_vertices[i] = _verts[i];
        }
    }
#endif

    // contains
    //   whether _p lies inside: the +x ray from it crosses an odd number of
    // edges.
    D_CONSTEXPR_CPP14
    bool contains(
        const point_type& _p
    ) const D_NOEXCEPT
    {
        value_type       xy[2 * N] = {};
        const point_type cart_p    = System::to_cartesian(_p);

        cartesian_vertices(xy);

        return internal::plane_polygon<value_type, N>::contains(
            xy, cart_p.data());
    }

    // closed_form_perimeter
    //   the edges' lengths, summed in order.
    D_CONSTEXPR_CPP14
    value_type closed_form_perimeter() const D_NOEXCEPT
    {
        value_type total = static_cast<value_type>(0);

        for (std::size_t i = 0; i < N; ++i)
        {
            total += cartesian_distance<System>(m_vertices[i],
                                                m_vertices[(i + 1) % N]);
        }

        return total;
    }

    // closed_form_area
    //   the shoelace formula's magnitude.
    D_CONSTEXPR_CPP14
    value_type closed_form_area() const D_NOEXCEPT
    {
        value_type xy[2 * N] = {};

        cartesian_vertices(xy);

        return internal::plane_polygon<value_type, N>::area(xy);
    }

    // closed_form_centroid
    //   the area-weighted centroid; the first vertex for a polygon of no
    // area.
    D_CONSTEXPR_CPP14
    point_type closed_form_centroid() const D_NOEXCEPT
    {
        value_type xy[2 * N] = {};
        point_type c;

        cartesian_vertices(xy);

        // no area: no centre to weigh
        if (!internal::plane_polygon<value_type, N>::centroid(c.begin(), xy))
        {
            return m_vertices[0];
        }

        return System::from_cartesian(c);
    }

private:
    // cartesian_vertices
    //   the vertices' Cartesian forms, interleaved into _xy.
    D_CONSTEXPR_CPP14
    void cartesian_vertices(
        value_type* _xy
    ) const D_NOEXCEPT
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            const point_type v = System::to_cartesian(m_vertices[i]);

            _xy[2 * i]       = v[0];
            _xy[(2 * i) + 1] = v[1];
        }

        return;
    }
};

template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t polygon<System, N>::dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t polygon<System, N>::vertex_count;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polygon<System, N>::is_shape;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polygon<System, N>::is_2d;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool polygon<System, N>::is_3d;


//==============================================================================
// 2.  PARAMETRIC REGIONS
//==============================================================================


// 2.1    parametric_region
//------------------------------------------------------------------------------
// 2.1.1
// parametric_region
//   struct: the region a closed parametric edge bounds.
template<typename Edge>
struct parametric_region
{
    D_STATIC_ASSERT((is_edge<Edge>::value),
                    "parametric_region: Edge must satisfy the edge "
                    "interface.");
    D_STATIC_ASSERT((Edge::dimension == 2),
                    "parametric_region: 2D edges only.");

    typedef Edge                              boundary_type;
    typedef typename Edge::coordinate_system  coordinate_system;
    typedef typename Edge::value_type         value_type;
    typedef typename Edge::point_type         point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = 2;
    static D_CONSTEXPR_VAR bool        is_shape  = true;
    static D_CONSTEXPR_VAR bool        is_2d     = true;
    static D_CONSTEXPR_VAR bool        is_3d     = false;

    boundary_type m_boundary;

    D_CONSTEXPR
    parametric_region() D_NOEXCEPT
        : m_boundary()
    {}

    D_CONSTEXPR explicit
    parametric_region(const boundary_type& _b) D_NOEXCEPT
        : m_boundary(_b)
    {}

    const boundary_type& boundary() const D_NOEXCEPT
    {
        return m_boundary;
    }

    // contains
    //   whether _p lies inside: the +x ray's crossings of the boundary,
    // sampled as Samples straight pieces, are odd (Samples 256 by default;
    // contains<N>(p) for another).
    template<std::size_t Samples>
    bool contains(
        const point_type& _p
    ) const
    {
        return ((internal::plane_edge_crossings<Samples, coordinate_system>(
                     m_boundary, coordinate_system::to_cartesian(_p)) % 2)
                == 1);
    }

    bool contains(
        const point_type& _p
    ) const
    {
        return contains<256>(_p);
    }
};

template<typename Edge>
D_CONSTEXPR_VAR std::size_t parametric_region<Edge>::dimension;
template<typename Edge>
D_CONSTEXPR_VAR bool parametric_region<Edge>::is_shape;
template<typename Edge>
D_CONSTEXPR_VAR bool parametric_region<Edge>::is_2d;
template<typename Edge>
D_CONSTEXPR_VAR bool parametric_region<Edge>::is_3d;


//==============================================================================
// 3.  IMPLICIT REGIONS
//==============================================================================
// From C++11: the expression evaluates on a std::tuple.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// 3.1    implicit_region
//------------------------------------------------------------------------------
// 3.1.1
// implicit_region
//   struct: the region F(x, y) <= 0, F a type with a static evaluate(std::
// tuple of x, y).
template<typename Expr,
         typename System = cartesian<2, double> >
struct implicit_region
{
    D_STATIC_ASSERT((System::dimension == 2),
                    "implicit_region: 2D coordinate systems only.");

    typedef System                      coordinate_system;
    typedef Expr                        expression_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static constexpr std::size_t dimension   = 2;
    static constexpr bool        is_shape    = true;
    static constexpr bool        is_2d       = true;
    static constexpr bool        is_3d       = false;
    static constexpr bool        is_implicit = true;

    // contains
    //   F(p) <= 0, on p's Cartesian form.
    bool
    contains(const point_type& _p) const
    {
        const point_type cart = System::to_cartesian(_p);

        return ( Expr::evaluate(std::make_tuple(cart[0], cart[1])) <=
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
        value_type       v    = Expr::evaluate(std::make_tuple(cart[0],
                                                               cart[1]));

        if (v < static_cast<value_type>(0))
        {
            v = -v;
        }

        return (v < _epsilon);
    }
};

template<typename Expr, typename System>
constexpr std::size_t implicit_region<Expr, System>::dimension;
template<typename Expr, typename System>
constexpr bool implicit_region<Expr, System>::is_shape;
template<typename Expr, typename System>
constexpr bool implicit_region<Expr, System>::is_2d;
template<typename Expr, typename System>
constexpr bool implicit_region<Expr, System>::is_3d;
template<typename Expr, typename System>
constexpr bool implicit_region<Expr, System>::is_implicit;


//==============================================================================
// 4.  SHAPES FROM EDGES
//==============================================================================
// From C++11: variadic.


// 4.1    shape_from_edges
//------------------------------------------------------------------------------
// 4.1.1
// shape_from_edges
//   struct: the region an ordered tuple of edges bounds together.
template<typename... Edges>
struct shape_from_edges
{
    static_assert(sizeof...(Edges) >= 1,
                  "shape_from_edges: at least one edge required.");

    typedef typename std::tuple_element<0, std::tuple<Edges...> >::type
        first_edge_type;
    typedef typename first_edge_type::coordinate_system coordinate_system;
    typedef typename coordinate_system::value_type      value_type;
    typedef typename coordinate_system::point_type      point_type;
    typedef std::tuple<Edges...>                        edges_type;

    static constexpr std::size_t dimension  = coordinate_system::dimension;
    static constexpr std::size_t edge_count = sizeof...(Edges);
    static constexpr bool        is_shape   = true;
    static constexpr bool        is_2d      = (dimension == 2);
    static constexpr bool        is_3d      = (dimension == 3);

    edges_type m_edges;

    constexpr
    shape_from_edges() noexcept
        : m_edges()
    {}

    constexpr explicit
    shape_from_edges(const Edges&... _e) noexcept
        : m_edges(_e...)
    {}

    const edges_type& edges() const noexcept
    {
        return m_edges;
    }

    // contains
    //   whether _p lies inside: the +x ray's crossings of every edge, each
    // sampled as SamplesPerEdge pieces, are odd in sum.
    template<std::size_t SamplesPerEdge>
    bool contains(
        const point_type& _p
    ) const
    {
        return ((count_crossings<SamplesPerEdge>(
                     coordinate_system::to_cartesian(_p),
                     re_std::make_index_sequence<sizeof...(Edges)>()) %
                 2) == 1);
    }

    bool contains(
        const point_type& _p
    ) const
    {
        return contains<32>(_p);
    }

private:
    // count_crossings
    //   each edge's crossings, in order, summed.
    template<std::size_t SPE,
             std::size_t... Is>
    std::size_t count_crossings(
        const point_type& _cart_p,
        re_std::index_sequence<Is...>
    ) const
    {
        const std::size_t counts[] = {
            internal::plane_edge_crossings<SPE, coordinate_system>(
                std::get<Is>(m_edges), _cart_p)...
        };
        std::size_t       total    = 0;

        for (std::size_t i = 0; i < sizeof...(Edges); ++i)
        {
            total += counts[i];
        }

        return total;
    }
};

template<typename... Edges>
constexpr std::size_t shape_from_edges<Edges...>::dimension;
template<typename... Edges>
constexpr std::size_t shape_from_edges<Edges...>::edge_count;
template<typename... Edges>
constexpr bool shape_from_edges<Edges...>::is_shape;
template<typename... Edges>
constexpr bool shape_from_edges<Edges...>::is_2d;
template<typename... Edges>
constexpr bool shape_from_edges<Edges...>::is_3d;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SHAPE_2D_HPP
