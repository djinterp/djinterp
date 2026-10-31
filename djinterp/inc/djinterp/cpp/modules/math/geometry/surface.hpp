/*******************************************************************************
* djinterp [cpp/math]                                                surface.hpp
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
*   - coordinate_system, value_type, point_type
*   - static std::size_t dimension                 (embedding)
*   - static std::size_t parametric_dimension = 2
*   - static bool is_surface = true
*   - static bool is_curved
*   - static bool is_closed
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
* LEVELS:
*   Everything compiles from C++98 but implicit_surface, which evaluates its
* expression on a std::tuple and so is C++11's. Points are linalg::vectors
* and the arithmetic theirs (c/math/vec.h), in the formulas' former order;
* a polygon's vertices are a re_std::array (from C++11 a std::array of
* points converts). closed_form_area and normal exist for 3D embeddings
* only, as before -- through a base class, which C++98 can spell, so that
* has_closed_form_area answers as it did.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/surface.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PLANAR SURFACES
    ---------------
    1.  triangle_surface
    2.  planar_polygon_surface
2.  CURVED SURFACES
    ---------------
    1.  bilinear_patch
    2.  parametric_surface_edge
    3.  implicit_surface
*/

#ifndef DJINTERP_MATH_GEOMETRY_SURFACE_HPP
#define DJINTERP_MATH_GEOMETRY_SURFACE_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // cartesian helpers
// re_std
#include "../../../../../re_std/cpp/array/array.hpp"        // re_std::array
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                              // std::array
    #include <tuple>                              // std::make_tuple
#endif


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  PLANAR SURFACES
//==============================================================================


NS_INTERNAL

    // triangle_surface_measures
    //   struct: a triangle's area and normal, which exist for a 3D embedding
    // only (the empty primary serves every other).
    template<typename System,
             typename Derived,
             bool     Is3D = (System::dimension == 3)>
    struct triangle_surface_measures
    {};

    template<typename System,
             typename Derived>
    struct triangle_surface_measures<System, Derived, true>
    {
        typedef typename System::value_type value_type;
        typedef typename System::point_type point_type;

        // closed_form_area
        //   |(v1 - v0) x (v2 - v0)| / 2.
        D_CONSTEXPR_CPP14
        value_type closed_form_area() const D_NOEXCEPT
        {
            const Derived&   self = static_cast<const Derived&>(*this);
            const point_type c0   = System::to_cartesian(self.m_v0);
            const point_type c1   = System::to_cartesian(self.m_v1);
            const point_type c2   = System::to_cartesian(self.m_v2);

            return c1.minus(c0).cross(c2.minus(c0)).norm() /
                   static_cast<value_type>(2);
        }

        // normal
        //   the unit normal (v1 - v0) x (v2 - v0) / |...|; zero for a
        // degenerate triangle.
        D_CONSTEXPR_CPP14
        linalg::vector<value_type, 3> normal() const D_NOEXCEPT
        {
            const Derived&   self = static_cast<const Derived&>(*this);
            const point_type c0   = System::to_cartesian(self.m_v0);
            const point_type c1   = System::to_cartesian(self.m_v1);
            const point_type c2   = System::to_cartesian(self.m_v2);

            return c1.minus(c0).cross(c2.minus(c0)).normalized();
        }
    };

    // planar_polygon_surface_measures
    //   struct: a planar polygon's area, for a 3D embedding only.
    template<typename    System,
             typename    Derived,
             std::size_t N,
             bool        Is3D = (System::dimension == 3)>
    struct planar_polygon_surface_measures
    {};

    template<typename    System,
             typename    Derived,
             std::size_t N>
    struct planar_polygon_surface_measures<System, Derived, N, true>
    {
        typedef typename System::value_type value_type;
        typedef typename System::point_type point_type;

        // closed_form_area
        //   half the length of the fan's summed cross products (v_i - v_0) x
        // (v_(i+1) - v_0), the polygon's vector area.
        D_CONSTEXPR_CPP14
        value_type closed_form_area() const D_NOEXCEPT
        {
            const Derived&   self = static_cast<const Derived&>(*this);
            const point_type c0   = System::to_cartesian(self.m_vertices[0]);
            point_type       n_total;

            for (std::size_t i = 1; (i + 1) < N; ++i)
            {
                const point_type c1 = System::to_cartesian(self.m_vertices[i]);
                const point_type c2 =
                    System::to_cartesian(self.m_vertices[i + 1]);

                n_total = n_total.plus(c1.minus(c0).cross(c2.minus(c0)));
            }

            return n_total.norm() / static_cast<value_type>(2);
        }
    };

NS_END  // internal

// 1.1    triangle_surface
//------------------------------------------------------------------------------
// 1.1.1
// triangle_surface
//   struct: the planar patch spanned by three vertices, at(u, v) = (1 - u -
// v) v0 + u v1 + v v2 over u, v in [0, 1].
template<typename System = cartesian<3, double> >
struct triangle_surface
    : internal::triangle_surface_measures<System, triangle_surface<System> >
{
    D_STATIC_ASSERT((System::dimension >= 3),
                    "triangle_surface: embedding dimension must be at "
                    "least 3 (use math::triangle for 2D).");

    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension            = System::dimension;
    static D_CONSTEXPR_VAR std::size_t parametric_dimension = 2;
    static D_CONSTEXPR_VAR bool        is_surface           = true;
    static D_CONSTEXPR_VAR bool        is_curved            = false;
    static D_CONSTEXPR_VAR bool        is_closed            = false;

    point_type m_v0;
    point_type m_v1;
    point_type m_v2;

    D_CONSTEXPR
    triangle_surface() D_NOEXCEPT
        : m_v0(),
          m_v1(),
          m_v2()
    {}

    D_CONSTEXPR
    triangle_surface(
        const point_type& _vertex_0,
        const point_type& _vertex_1,
        const point_type& _vertex_2
    ) D_NOEXCEPT
        : m_v0(_vertex_0),
          m_v1(_vertex_1),
          m_v2(_vertex_2)
    {}

    D_CONSTEXPR value_type
    parameter_min_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    D_CONSTEXPR value_type
    parameter_min_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // at
    //   (1 - u - v) v0 + u v1 + v v2, through Cartesian.
    D_CONSTEXPR_CPP14 point_type
    at(
        value_type _u,
        value_type _v
    ) const D_NOEXCEPT
    {
        const value_type w = (static_cast<value_type>(1) - _u) - _v;

        return System::from_cartesian(
            System::to_cartesian(m_v0).scaled(w)
                .plus(System::to_cartesian(m_v1).scaled(_u))
                .plus(System::to_cartesian(m_v2).scaled(_v)));
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t triangle_surface<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR std::size_t triangle_surface<System>::parametric_dimension;
template<typename System>
D_CONSTEXPR_VAR bool triangle_surface<System>::is_surface;
template<typename System>
D_CONSTEXPR_VAR bool triangle_surface<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool triangle_surface<System>::is_closed;

// 1.2    planar_polygon_surface
//------------------------------------------------------------------------------
// 1.2.1
// planar_polygon_surface
//   struct: N coplanar vertices, a planar patch.
template<typename    System,
         std::size_t N>
struct planar_polygon_surface
    : internal::planar_polygon_surface_measures<
          System, planar_polygon_surface<System, N>, N>
{
    D_STATIC_ASSERT((System::dimension >= 3),
                    "planar_polygon_surface: embedding dimension must be "
                    "at least 3 (use math::polygon for 2D).");
    D_STATIC_ASSERT((N >= 3),
                    "planar_polygon_surface: at least 3 vertices required.");

    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;
    typedef re_std::array<point_type, N> vertex_array;

    static D_CONSTEXPR_VAR std::size_t dimension            = System::dimension;
    static D_CONSTEXPR_VAR std::size_t parametric_dimension = 2;
    static D_CONSTEXPR_VAR std::size_t vertex_count         = N;
    static D_CONSTEXPR_VAR bool        is_surface           = true;
    static D_CONSTEXPR_VAR bool        is_curved            = false;
    static D_CONSTEXPR_VAR bool        is_closed            = false;

    vertex_array m_vertices;

    D_CONSTEXPR
    planar_polygon_surface() D_NOEXCEPT
        : m_vertices()
    {}

    D_CONSTEXPR explicit
    planar_polygon_surface(const vertex_array& _verts) D_NOEXCEPT
        : m_vertices(_verts)
    {}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from a std::array of points (C++11), as the vertices were held
    D_CONSTEXPR_CPP14 explicit
    planar_polygon_surface(const std::array<point_type, N>& _verts) D_NOEXCEPT
        : m_vertices()
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            m_vertices[i] = _verts[i];
        }
    }
#endif

    D_CONSTEXPR
    value_type parameter_min_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    D_CONSTEXPR
    value_type parameter_min_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR
    value_type parameter_max_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }
};

template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t planar_polygon_surface<System, N>::dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t
    planar_polygon_surface<System, N>::parametric_dimension;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR std::size_t planar_polygon_surface<System, N>::vertex_count;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool planar_polygon_surface<System, N>::is_surface;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool planar_polygon_surface<System, N>::is_curved;
template<typename System, std::size_t N>
D_CONSTEXPR_VAR bool planar_polygon_surface<System, N>::is_closed;


//==============================================================================
// 2.  CURVED SURFACES
//==============================================================================


// 2.1    bilinear_patch
//------------------------------------------------------------------------------
// 2.1.1
// bilinear_patch
//   struct: the ruled surface through four corners, at(u, v) = (1-u)(1-v)
// p00 + u(1-v) p10 + (1-u)v p01 + uv p11.
template<typename System = cartesian<3, double> >
struct bilinear_patch
{
    D_STATIC_ASSERT((System::dimension >= 3),
                    "bilinear_patch: embedding dimension must be at least "
                    "3.");

    typedef System                      coordinate_system;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension            = System::dimension;
    static D_CONSTEXPR_VAR std::size_t parametric_dimension = 2;
    static D_CONSTEXPR_VAR bool        is_surface           = true;
    static D_CONSTEXPR_VAR bool        is_curved            = true;
    static D_CONSTEXPR_VAR bool        is_closed            = false;

    point_type m_p00;
    point_type m_p10;
    point_type m_p01;
    point_type m_p11;

    D_CONSTEXPR
    bilinear_patch() D_NOEXCEPT
        : m_p00(),
          m_p10(),
          m_p01(),
          m_p11()
    {}

    D_CONSTEXPR
    bilinear_patch(
        const point_type& _corner_00,
        const point_type& _corner_10,
        const point_type& _corner_01,
        const point_type& _corner_11
    ) D_NOEXCEPT
        : m_p00(_corner_00),
          m_p10(_corner_10),
          m_p01(_corner_01),
          m_p11(_corner_11)
    {}

    D_CONSTEXPR value_type
    parameter_min_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    D_CONSTEXPR value_type
    parameter_min_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // at
    //   the corners weighted (1-u)(1-v), u(1-v), (1-u)v and uv, summed in
    // that order, through Cartesian.
    D_CONSTEXPR_CPP14 point_type
    at(
        value_type _u,
        value_type _v
    ) const D_NOEXCEPT
    {
        const value_type uu  = static_cast<value_type>(1) - _u;
        const value_type vv  = static_cast<value_type>(1) - _v;
        const value_type w00 = uu * vv;
        const value_type w10 = _u * vv;
        const value_type w01 = uu * _v;
        const value_type w11 = _u * _v;

        return System::from_cartesian(
            System::to_cartesian(m_p00).scaled(w00)
                .plus(System::to_cartesian(m_p10).scaled(w10))
                .plus(System::to_cartesian(m_p01).scaled(w01))
                .plus(System::to_cartesian(m_p11).scaled(w11)));
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t bilinear_patch<System>::dimension;
template<typename System>
D_CONSTEXPR_VAR std::size_t bilinear_patch<System>::parametric_dimension;
template<typename System>
D_CONSTEXPR_VAR bool bilinear_patch<System>::is_surface;
template<typename System>
D_CONSTEXPR_VAR bool bilinear_patch<System>::is_curved;
template<typename System>
D_CONSTEXPR_VAR bool bilinear_patch<System>::is_closed;

// 2.2    parametric_surface_edge
//------------------------------------------------------------------------------
// 2.2.1
// parametric_surface_edge
//   struct: wraps a parametric surface type -- a static evaluate(u, v)
// returning an indexable point, and a static output_dimension -- over a
// parameter rectangle.
template<typename ParametricSurface,
         typename System = cartesian<ParametricSurface::output_dimension,
                                     double> >
struct parametric_surface_edge
{
    typedef System                      coordinate_system;
    typedef ParametricSurface           parametric_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension            = System::dimension;
    static D_CONSTEXPR_VAR std::size_t parametric_dimension = 2;
    static D_CONSTEXPR_VAR bool        is_surface           = true;
    static D_CONSTEXPR_VAR bool        is_curved            = true;
    static D_CONSTEXPR_VAR bool        is_closed            = false;

    D_STATIC_ASSERT((ParametricSurface::output_dimension == System::dimension),
                    "parametric_surface_edge: surface output dimension must "
                    "match coordinate-system dimension.");

    value_type m_u_min;
    value_type m_u_max;
    value_type m_v_min;
    value_type m_v_max;

    D_CONSTEXPR
    parametric_surface_edge() D_NOEXCEPT
        : m_u_min(static_cast<value_type>(0)),
          m_u_max(static_cast<value_type>(1)),
          m_v_min(static_cast<value_type>(0)),
          m_v_max(static_cast<value_type>(1))
    {}

    D_CONSTEXPR
    parametric_surface_edge(
        value_type _u_lower,
        value_type _u_upper,
        value_type _v_lower,
        value_type _v_upper
    ) D_NOEXCEPT
        : m_u_min(_u_lower),
          m_u_max(_u_upper),
          m_v_min(_v_lower),
          m_v_max(_v_upper)
    {}

    D_CONSTEXPR value_type
    parameter_min_u() const D_NOEXCEPT
    {
        return m_u_min;
    }

    D_CONSTEXPR value_type
    parameter_max_u() const D_NOEXCEPT
    {
        return m_u_max;
    }

    D_CONSTEXPR value_type
    parameter_min_v() const D_NOEXCEPT
    {
        return m_v_min;
    }

    D_CONSTEXPR value_type
    parameter_max_v() const D_NOEXCEPT
    {
        return m_v_max;
    }

    // at
    //   the wrapped surface's point at (u, v), in this system (a Cartesian
    // system's from_cartesian is the point itself).
    point_type
    at(
        value_type _u,
        value_type _v
    ) const
    {
        return from_raw(ParametricSurface::evaluate(_u, _v));
    }

private:
    // from_raw
    //   the evaluated point, its components converted -- a template, so the
    // result's type is deduced at every level.
    template<typename Raw>
    static point_type
    from_raw(const Raw& _raw)
    {
        point_type cart;

        for (std::size_t i = 0; i < dimension; ++i)
        {
            cart[i] = static_cast<value_type>(_raw[i]);
        }

        return System::from_cartesian(cart);
    }
};

template<typename ParametricSurface, typename System>
D_CONSTEXPR_VAR std::size_t
    parametric_surface_edge<ParametricSurface, System>::dimension;
template<typename ParametricSurface, typename System>
D_CONSTEXPR_VAR std::size_t
    parametric_surface_edge<ParametricSurface, System>::parametric_dimension;
template<typename ParametricSurface, typename System>
D_CONSTEXPR_VAR bool
    parametric_surface_edge<ParametricSurface, System>::is_surface;
template<typename ParametricSurface, typename System>
D_CONSTEXPR_VAR bool
    parametric_surface_edge<ParametricSurface, System>::is_curved;
template<typename ParametricSurface, typename System>
D_CONSTEXPR_VAR bool
    parametric_surface_edge<ParametricSurface, System>::is_closed;

// 2.3    implicit_surface
//------------------------------------------------------------------------------
// From C++11: its expression evaluates on a std::tuple.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// 2.3.1
// implicit_surface
//   struct: the zero level set of an expression F(x, y, z) -- a type with a
// static evaluate(std::tuple of x, y, z) -- in 3D.
template<typename Expr,
         typename System = cartesian<3, double> >
struct implicit_surface
{
    D_STATIC_ASSERT((System::dimension == 3),
                    "implicit_surface: 3D coordinate systems only.");

    typedef System                      coordinate_system;
    typedef Expr                        expression_type;
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    static D_CONSTEXPR_VAR std::size_t dimension            = 3;
    static D_CONSTEXPR_VAR std::size_t parametric_dimension = 2;
    static D_CONSTEXPR_VAR bool        is_surface           = true;
    static D_CONSTEXPR_VAR bool        is_curved            = true;
    static D_CONSTEXPR_VAR bool        is_closed            = false;
    static D_CONSTEXPR_VAR bool        is_implicit          = true;

    D_CONSTEXPR value_type
    parameter_min_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_u() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    D_CONSTEXPR value_type
    parameter_min_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(0);
    }

    D_CONSTEXPR value_type
    parameter_max_v() const D_NOEXCEPT
    {
        return static_cast<value_type>(1);
    }

    // on_surface
    //   whether |F(p)| < _epsilon, F evaluated on p's Cartesian form.
    bool
    on_surface(
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
constexpr std::size_t implicit_surface<Expr, System>::dimension;
template<typename Expr, typename System>
constexpr std::size_t implicit_surface<Expr, System>::parametric_dimension;
template<typename Expr, typename System>
constexpr bool implicit_surface<Expr, System>::is_surface;
template<typename Expr, typename System>
constexpr bool implicit_surface<Expr, System>::is_curved;
template<typename Expr, typename System>
constexpr bool implicit_surface<Expr, System>::is_closed;
template<typename Expr, typename System>
constexpr bool implicit_surface<Expr, System>::is_implicit;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_SURFACE_HPP
