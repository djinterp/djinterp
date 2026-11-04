/*******************************************************************************
* djinterp [cpp/math]                                             measure_3d.hpp
*
* 3D measurements with closed-form / numerical dispatch.
*   Provides free function templates that compute geometric quantities
* of surfaces and 3D shapes. Each measurement prefers a closed-form
* method on the operand when one is present (detected via the
* has_closed_form_* traits in geometry_common.hpp); when no closed
* form is available, it falls back to numerical procedures built on
* composite Simpson's rule (1D for edges, 2D iterated for surfaces)
* and the divergence theorem (for free-form volumes).
*
* PROVIDED FUNCTIONS:
*   patch_area<Surface>(s)        - area of a single surface patch
*   volume<Shape>(s)              - volume of a 3D shape
*   surface_area<Shape>(s)        - total bounding surface area
*   centroid_3d<Shape>(s)         - geometric centroid (closed-form only)
*   bounding_box_of_surface(...)  - sampled AABB for a surface
*   bounding_box_of_sphere(...)   - exact AABB for a sphere
*   bounding_box_of_box(...)      - exact AABB for an axis-aligned box
*   bounding_box_of_cylinder(...) - exact AABB for a cylinder
*   volume_divergence(...)        - divergence-theorem volume of a
*                                    closed parametric surface
*
* LEVELS:
*   Everything compiles from C++98: the closed-form dispatch is a bool
* template parameter where it was an enable_if_t default; the iterated
* Simpson integrands are function objects where they were nested lambdas,
* their central differences, cross products and order unchanged; the
* requirement checks sit at class scope; centroid_3d keeps its enable_if,
* re_std's, which every level has.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/measure_3d.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  AREAS
    -----
    1.  patch_area
    2.  surface_area
2.  VOLUMES AND CENTROIDS
    ---------------------
    1.  volume
    2.  volume_divergence
    3.  centroid_3d
3.  BOUNDING BOXES
    --------------
*/

#ifndef DJINTERP_MATH_GEOMETRY_MEASURE_3D_HPP
#define DJINTERP_MATH_GEOMETRY_MEASURE_3D_HPP 1

// std
#include <cmath>                                  // std::sqrt
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "./geometry_common.hpp"                  // the traits, aabb,
                                                  // integrate_simpson
#include "./named_3d.hpp"                         // sphere, box, cylinder
#include "./solid.hpp"                            // the solids
#include "./surface.hpp"                          // the surfaces
// re_std
#include "../../../../../re_std/cpp/type_traits/enable_if.hpp"  // enable_if


NS_DJINTERP
NS_MATH


NS_INTERNAL

    // surface_tangents
    //   struct: a surface's tangents r_u and r_v at (u, v), by central
    // differences of half-widths m_eps_u and m_eps_v, and their cross
    // product -- the shared part of both surface integrands.
    template<typename Surface>
    struct surface_tangents
    {
        typedef typename Surface::value_type        value_type;
        typedef typename Surface::coordinate_system system_type;
        typedef typename Surface::point_type        point_type;

        value_type cx;
        value_type cy;
        value_type cz;

        surface_tangents(
            const Surface& _s,
            value_type     _u,
            value_type     _v,
            value_type     _eps_u,
            value_type     _eps_v
        )
            : cx(),
              cy(),
              cz()
        {
            const point_type p_u_fwd = system_type::to_cartesian(
                                           _s.at(_u + _eps_u, _v));
            const point_type p_u_bck = system_type::to_cartesian(
                                           _s.at(_u - _eps_u, _v));
            const point_type p_v_fwd = system_type::to_cartesian(
                                           _s.at(_u, _v + _eps_v));
            const point_type p_v_bck = system_type::to_cartesian(
                                           _s.at(_u, _v - _eps_v));
            const value_type two_eu  = static_cast<value_type>(2) * _eps_u;
            const value_type two_ev  = static_cast<value_type>(2) * _eps_v;
            const value_type rux     = (p_u_fwd[0] - p_u_bck[0]) / two_eu;
            const value_type ruy     = (p_u_fwd[1] - p_u_bck[1]) / two_eu;
            const value_type ruz     = (p_u_fwd[2] - p_u_bck[2]) / two_eu;
            const value_type rvx     = (p_v_fwd[0] - p_v_bck[0]) / two_ev;
            const value_type rvy     = (p_v_fwd[1] - p_v_bck[1]) / two_ev;
            const value_type rvz     = (p_v_fwd[2] - p_v_bck[2]) / two_ev;

            cx = (ruy * rvz) - (ruz * rvy);
            cy = (ruz * rvx) - (rux * rvz);
            cz = (rux * rvy) - (ruy * rvx);
        }
    };

    // area_integrand
    //   struct: |r_u x r_v|, the area element.
    template<typename Surface>
    struct area_integrand
    {
        typedef typename Surface::value_type value_type;

        const Surface& m_s;
        value_type     m_eps_u;
        value_type     m_eps_v;

        area_integrand(
            const Surface& _s,
            value_type     _eps_u,
            value_type     _eps_v
        )
            : m_s(_s),
              m_eps_u(_eps_u),
              m_eps_v(_eps_v)
        {}

        value_type
        operator()(value_type _u,
                   value_type _v) const
        {
            const surface_tangents<Surface> t(m_s, _u, _v, m_eps_u, m_eps_v);

            return std::sqrt((t.cx * t.cx) + (t.cy * t.cy) + (t.cz * t.cz));
        }
    };

    // divergence_integrand
    //   struct: p · (r_u x r_v) / 3, the divergence theorem's integrand for
    // the field p.
    template<typename Surface>
    struct divergence_integrand
    {
        typedef typename Surface::value_type        value_type;
        typedef typename Surface::coordinate_system system_type;
        typedef typename Surface::point_type        point_type;

        const Surface& m_s;
        value_type     m_eps_u;
        value_type     m_eps_v;

        divergence_integrand(
            const Surface& _s,
            value_type     _eps_u,
            value_type     _eps_v
        )
            : m_s(_s),
              m_eps_u(_eps_u),
              m_eps_v(_eps_v)
        {}

        value_type
        operator()(value_type _u,
                   value_type _v) const
        {
            const point_type                p = system_type::to_cartesian(
                                                    m_s.at(_u, _v));
            const surface_tangents<Surface> t(m_s, _u, _v, m_eps_u, m_eps_v);

            return ((p[0] * t.cx) + (p[1] * t.cy) + (p[2] * t.cz)) /
                   static_cast<value_type>(3);
        }
    };

    // simpson_inner, simpson_outer
    //   struct: the iterated Simpson rule's two levels -- for a fixed u, the
    // integral over v; then the integral of that over u.
    template<typename Integrand>
    struct simpson_inner
    {
        typedef typename Integrand::value_type value_type;

        const Integrand& m_f;
        value_type       m_u;

        simpson_inner(
            const Integrand& _f,
            value_type       _u
        )
            : m_f(_f),
              m_u(_u)
        {}

        value_type
        operator()(value_type _v) const
        {
            return m_f(m_u, _v);
        }
    };

    template<typename Integrand>
    struct simpson_outer
    {
        typedef typename Integrand::value_type value_type;

        const Integrand& m_f;
        value_type       m_v_min;
        value_type       m_v_max;
        std::size_t      m_nv;

        simpson_outer(
            const Integrand& _f,
            value_type       _v_min,
            value_type       _v_max,
            std::size_t      _nv
        )
            : m_f(_f),
              m_v_min(_v_min),
              m_v_max(_v_max),
              m_nv(_nv)
        {}

        value_type
        operator()(value_type _u) const
        {
            return integrate_simpson(simpson_inner<Integrand>(m_f, _u),
                                     m_v_min,
                                     m_v_max,
                                     m_nv);
        }
    };

    // simpson_2d
    //   function: the iterated Simpson integral of _f over a surface's
    // parameter rectangle, _nu by _nv intervals.
    template<typename Integrand,
             typename Surface>
    typename Surface::value_type
    simpson_2d(
        const Integrand& _f,
        const Surface&   _s,
        std::size_t      _nu,
        std::size_t      _nv
    )
    {
        return integrate_simpson(
                   simpson_outer<Integrand>(_f,
                                            _s.parameter_min_v(),
                                            _s.parameter_max_v(),
                                            _nv),
                   _s.parameter_min_u(),
                   _s.parameter_max_u(),
                   _nu);
    }

    // the central differences' half-widths: 1/64 of a Simpson interval
    template<typename Surface>
    typename Surface::value_type
    surface_eps_u(
        const Surface& _s,
        std::size_t    _nu
    )
    {
        typedef typename Surface::value_type value_type;

        return (_s.parameter_max_u() - _s.parameter_min_u()) /
               static_cast<value_type>(_nu * 64);
    }

    template<typename Surface>
    typename Surface::value_type
    surface_eps_v(
        const Surface& _s,
        std::size_t    _nv
    )
    {
        typedef typename Surface::value_type value_type;

        return (_s.parameter_max_v() - _s.parameter_min_v()) /
               static_cast<value_type>(_nv * 64);
    }

    // surface_area_simpson_2d
    //   function: a patch's area, the iterated Simpson integral of |r_u x
    // r_v|.
    template<typename Surface>
    typename Surface::value_type
    surface_area_simpson_2d(
        const Surface& _s,
        std::size_t    _nu,
        std::size_t    _nv
    )
    {
        return simpson_2d(area_integrand<Surface>(_s,
                                                  surface_eps_u(_s, _nu),
                                                  surface_eps_v(_s, _nv)),
                          _s,
                          _nu,
                          _nv);
    }

    // measure_3d's requirement checks, asserted at class scope, once for
    // every function that names them
    template<typename Surface>
    struct measure_surface_check
    {
        D_STATIC_ASSERT((is_surface<Surface>::value),
                        "measure_3d: Surface must satisfy the surface "
                        "interface.");

        enum
        {
            ok = 1
        };
    };

    template<typename Surface>
    struct measure_surface_3d_check
    {
        D_STATIC_ASSERT((measure_surface_check<Surface>::ok == 1) &&
                        (Surface::dimension == 3),
                        "measure_3d: 3D surfaces only.");

        enum
        {
            ok = 1
        };
    };

    template<typename Shape>
    struct measure_shape_3d_check
    {
        D_STATIC_ASSERT((is_shape<Shape>::value),
                        "measure_3d: Shape must satisfy the shape "
                        "interface.");
        D_STATIC_ASSERT((Shape::is_3d),
                        "measure_3d: 3D shapes only (use perimeter() and "
                        "area() for 2D).");

        enum
        {
            ok = 1
        };
    };

    // patch_area_impl, volume_impl, surface_area_impl, centroid_3d_impl
    //   struct: the closed form where the operand has one; a patch's area
    // by Simpson's rule otherwise, and for the rest a compile-time error
    // saying what to provide.
    template<typename Surface,
             bool     Closed = has_closed_form_area<Surface>::value>
    struct patch_area_impl
    {
        static typename Surface::value_type
        compute(
            const Surface& _s,
            std::size_t    _nu,
            std::size_t    _nv
        )
        {
            return surface_area_simpson_2d(_s, _nu, _nv);
        }
    };

    template<typename Surface>
    struct patch_area_impl<Surface, true>
    {
        static typename Surface::value_type
        compute(
            const Surface& _s,
            std::size_t,
            std::size_t
        )
        {
            return _s.closed_form_area();
        }
    };

    template<typename Shape,
             bool     Closed = has_closed_form_volume<Shape>::value>
    struct volume_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "volume: no closed-form volume available. Provide "
                        "closed_form_volume() on the shape, or call "
                        "volume_divergence() with an explicit boundary "
                        "surface.");
    };

    template<typename Shape>
    struct volume_impl<Shape, true>
    {
        static typename Shape::value_type
        compute(const Shape& _s)
        {
            return _s.closed_form_volume();
        }
    };

    template<typename Shape,
             bool     Closed = has_closed_form_surface_area<Shape>::value>
    struct surface_area_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "surface_area: no closed-form surface area "
                        "available. Provide closed_form_surface_area() on "
                        "the shape, or sum patch_area() over the boundary "
                        "surfaces.");
    };

    template<typename Shape>
    struct surface_area_impl<Shape, true>
    {
        static typename Shape::value_type
        compute(const Shape& _s)
        {
            return _s.closed_form_surface_area();
        }
    };

    template<typename Shape,
             bool     Closed = has_closed_form_centroid<Shape>::value>
    struct centroid_3d_impl
    {
        D_STATIC_ASSERT((sizeof(Shape) == 0),
                        "centroid: no closed_form_centroid() on this shape "
                        "and no general numerical fallback. Define "
                        "closed_form_centroid().");
    };

    template<typename Shape>
    struct centroid_3d_impl<Shape, true>
    {
        static typename Shape::point_type
        compute(const Shape& _s)
        {
            return _s.closed_form_centroid();
        }
    };

NS_END  // internal


//==============================================================================
// 1.  AREAS
//==============================================================================


// 1.1    patch_area
//------------------------------------------------------------------------------
// 1.1.1
// patch_area
//   function: a surface patch's area -- its closed form, or the iterated
// Simpson integral of |r_u x r_v| over _nu by _nv intervals.
template<typename Surface>
typename Surface::value_type
patch_area(
    const Surface& _s,
    std::size_t    _nu = 32,
    std::size_t    _nv = 32
)
{
    (void)internal::measure_surface_check<Surface>::ok;

    return internal::patch_area_impl<Surface>::compute(_s, _nu, _nv);
}

// 1.2    surface_area
//------------------------------------------------------------------------------
// 1.2.1
// surface_area
//   function: a 3D shape's surface area, its closed form.
template<typename Shape>
typename Shape::value_type
surface_area(const Shape& _s)
{
    (void)internal::measure_shape_3d_check<Shape>::ok;

    return internal::surface_area_impl<Shape>::compute(_s);
}


//==============================================================================
// 2.  VOLUMES AND CENTROIDS
//==============================================================================


// 2.1    volume
//------------------------------------------------------------------------------
// 2.1.1
// volume
//   function: a 3D shape's volume, its closed form.
template<typename Shape>
typename Shape::value_type
volume(const Shape& _s)
{
    (void)internal::measure_shape_3d_check<Shape>::ok;

    return internal::volume_impl<Shape>::compute(_s);
}

// 2.2    volume_divergence
//------------------------------------------------------------------------------
// 2.2.1
// volume_divergence
//   function: the volume a closed surface bounds, |∬ p · (r_u x r_v) / 3|
// by the iterated Simpson rule over _nu by _nv intervals.
template<typename Surface>
typename Surface::value_type
volume_divergence(
    const Surface& _boundary,
    std::size_t    _nu = 32,
    std::size_t    _nv = 32
)
{
    typedef typename Surface::value_type value_type;

    (void)internal::measure_surface_3d_check<Surface>::ok;

    const value_type result = internal::simpson_2d(
        internal::divergence_integrand<Surface>(
            _boundary,
            internal::surface_eps_u(_boundary, _nu),
            internal::surface_eps_v(_boundary, _nv)),
        _boundary,
        _nu,
        _nv);

    return (result < static_cast<value_type>(0)) ? -result : result;
}

// 2.3    centroid_3d
//------------------------------------------------------------------------------
// 2.3.1
// centroid_3d
//   function: a 3D shape's centroid, its closed form (present only for 3D
// shapes).
template<typename Shape>
typename re_std::enable_if<Shape::is_3d, typename Shape::point_type>::type
centroid_3d(const Shape& _s)
{
    (void)internal::measure_shape_3d_check<Shape>::ok;

    return internal::centroid_3d_impl<Shape>::compute(_s);
}


//==============================================================================
// 3.  BOUNDING BOXES
//==============================================================================


// bounding_box_of_surface
//   function: the Cartesian box of (_nu + 1)(_nv + 1) points on a surface.
template<typename Surface>
aabb<3, typename Surface::value_type>
bounding_box_of_surface(
    const Surface& _s,
    std::size_t    _nu = 16,
    std::size_t    _nv = 16
)
{
    typedef typename Surface::value_type        value_type;
    typedef typename Surface::coordinate_system system_type;

    (void)internal::measure_surface_3d_check<Surface>::ok;

    const value_type    u_min = _s.parameter_min_u();
    const value_type    v_min = _s.parameter_min_v();
    const value_type    du    = (_s.parameter_max_u() - u_min) /
                                static_cast<value_type>(_nu);
    const value_type    dv    = (_s.parameter_max_v() - v_min) /
                                static_cast<value_type>(_nv);
    aabb<3, value_type> box   = aabb<3, value_type>::empty();

    for (std::size_t i = 0; i <= _nu; ++i)
    {
        const value_type u = u_min + (static_cast<value_type>(i) * du);

        for (std::size_t j = 0; j <= _nv; ++j)
        {
            box.include(system_type::to_cartesian(
                _s.at(u, v_min + (static_cast<value_type>(j) * dv))));
        }
    }

    return box;
}

// bounding_box_of_sphere, _of_box, _of_cylinder
//   function: the exact Cartesian boxes of the named solids.
template<typename System>
aabb<3, typename System::value_type>
bounding_box_of_sphere(const sphere<System>& _sph)
{
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    const point_type cc = System::to_cartesian(_sph.m_center);

    return aabb<3, value_type>(point_type(cc[0] - _sph.m_radius,
                                          cc[1] - _sph.m_radius,
                                          cc[2] - _sph.m_radius),
                               point_type(cc[0] + _sph.m_radius,
                                          cc[1] + _sph.m_radius,
                                          cc[2] + _sph.m_radius));
}

template<typename System>
aabb<3, typename System::value_type>
bounding_box_of_box(const box<System>& _b)
{
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    const point_type cc = System::to_cartesian(_b.m_corner);

    return aabb<3, value_type>(cc,
                               point_type(cc[0] + _b.m_width,
                                          cc[1] + _b.m_height,
                                          cc[2] + _b.m_depth));
}

template<typename System>
aabb<3, typename System::value_type>
bounding_box_of_cylinder(const cylinder<System>& _cyl)
{
    typedef typename System::value_type value_type;
    typedef typename System::point_type point_type;

    const point_type bc = System::to_cartesian(_cyl.m_base);

    return aabb<3, value_type>(point_type(bc[0] - _cyl.m_radius,
                                          bc[1] - _cyl.m_radius,
                                          bc[2]),
                               point_type(bc[0] + _cyl.m_radius,
                                          bc[1] + _cyl.m_radius,
                                          bc[2] + _cyl.m_height));
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_MEASURE_3D_HPP
