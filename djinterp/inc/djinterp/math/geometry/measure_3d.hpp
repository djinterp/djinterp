/******************************************************************************
* djinterp [math]                                              measure_3d.hpp
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
*   centroid<Shape>(s)            - geometric centroid (closed-form only)
*   bounding_box_of_surface(...)  - sampled AABB for a surface
*   bounding_box_of_sphere(...)   - exact AABB for a sphere
*   bounding_box_of_box(...)      - exact AABB for an axis-aligned box
*   volume_divergence(...)        - divergence-theorem volume of a
*                                    closed parametric surface
*
* NOTE: a 2D centroid() lives in measure_2d.hpp; this module provides
* its own centroid() over 3D shapes. The two are not in conflict
* because each fires only when its target shape has matching is_2d /
* is_3d flags.
*
* 
* path:      /inc/djinterp/math/geometry/measure_3d.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_MEASURE_3D_
#define DJINTERP_MATH_GEOMETRY_MEASURE_3D_ 1

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
#include "./surface.hpp"
#include "./solid.hpp"
#include "./named_3d.hpp"


NS_DJINTERP
NS_MATH


// ============================================================================
// I.    SURFACE PATCH AREA
// ============================================================================

NS_INTERNAL

    // surface_area_simpson_2d
    //   helper: composite Simpson's rule in two dimensions for the
    // surface-area integral
    //   A = ∫∫ |r_u × r_v| du dv
    // where r_u and r_v are partial derivatives of the surface
    // parameterisation. Both are approximated by central differences
    // in Cartesian space.
    template<typename _Surface>
    typename _Surface::value_type
    surface_area_simpson_2d
    (
        const _Surface& _s,
        std::size_t     _nu,
        std::size_t     _nv
    ) noexcept
    {
        using value_type = typename _Surface::value_type;
        using sys        = typename _Surface::coordinate_system;

        value_type u_min;
        value_type u_max;
        value_type v_min;
        value_type v_max;
        value_type eps_u;
        value_type eps_v;

        u_min = _s.parameter_min_u();
        u_max = _s.parameter_max_u();
        v_min = _s.parameter_min_v();
        v_max = _s.parameter_max_v();

        // central-difference offsets proportional to step
        eps_u = (u_max - u_min) /
                static_cast<value_type>(_nu * 64);
        eps_v = (v_max - v_min) /
                static_cast<value_type>(_nv * 64);

        // |r_u × r_v| at (u, v), evaluated in Cartesian
        auto integrand =
            [&_s, eps_u, eps_v]
            (value_type _u, value_type _v) -> value_type
            {
                auto p_u_fwd = sys::to_cartesian(
                    _s.at(_u + eps_u, _v));
                auto p_u_bck = sys::to_cartesian(
                    _s.at(_u - eps_u, _v));
                auto p_v_fwd = sys::to_cartesian(
                    _s.at(_u, _v + eps_v));
                auto p_v_bck = sys::to_cartesian(
                    _s.at(_u, _v - eps_v));

                value_type two_eu = static_cast<value_type>(2) *
                                    eps_u;
                value_type two_ev = static_cast<value_type>(2) *
                                    eps_v;

                value_type rux = (p_u_fwd[0] - p_u_bck[0]) / two_eu;
                value_type ruy = (p_u_fwd[1] - p_u_bck[1]) / two_eu;
                value_type ruz = (p_u_fwd[2] - p_u_bck[2]) / two_eu;
                value_type rvx = (p_v_fwd[0] - p_v_bck[0]) / two_ev;
                value_type rvy = (p_v_fwd[1] - p_v_bck[1]) / two_ev;
                value_type rvz = (p_v_fwd[2] - p_v_bck[2]) / two_ev;

                // cross product
                value_type cx = ruy * rvz - ruz * rvy;
                value_type cy = ruz * rvx - rux * rvz;
                value_type cz = rux * rvy - ruy * rvx;

                return std::sqrt(cx * cx + cy * cy + cz * cz);
            };

        // outer Simpson in u of (inner Simpson in v)
        auto outer = [&integrand, v_min, v_max, _nv]
                     (value_type _u) -> value_type
        {
            auto inner = [&integrand, _u]
                         (value_type _v) -> value_type
            {
                return integrand(_u, _v);
            };

            return integrate_simpson<decltype(inner), value_type>(
                inner, v_min, v_max, _nv);
        };

        return integrate_simpson<decltype(outer), value_type>(
            outer, u_min, u_max, _nu);
    }

    // patch_area_impl (general fallback)
    //   helper: numerical 2D Simpson when no closed_form_area exists.
    template<typename _Surface,
             typename = void>
    struct patch_area_impl
    {
        using value_type = typename _Surface::value_type;

        static value_type
        compute
        (
            const _Surface& _s,
            std::size_t     _nu,
            std::size_t     _nv
        ) noexcept
        {
            return surface_area_simpson_2d(_s, _nu, _nv);
        }
    };

    // patch_area_impl (closed-form specialisation)
    template<typename _Surface>
    struct patch_area_impl<_Surface, std::enable_if_t<
        has_closed_form_area<_Surface>::value
    >>
    {
        using value_type = typename _Surface::value_type;

        static value_type
        compute
        (
            const _Surface& _s,
            std::size_t,
            std::size_t
        ) noexcept
        {
            return _s.closed_form_area();
        }
    };

NS_END  // internal

// patch_area
//   computes the area of a single surface patch. Uses
// closed_form_area when available, otherwise composite 2D Simpson
// with _nu × _nv subintervals.
template<typename _Surface>
typename _Surface::value_type
patch_area
(
    const _Surface& _s,
    std::size_t     _nu = 32,
    std::size_t     _nv = 32
) noexcept
{
    static_assert(is_surface<_Surface>::value,
                  "patch_area: _Surface must satisfy the surface "
                  "interface.");

    return internal::patch_area_impl<_Surface>::compute(_s, _nu, _nv);
}


// ============================================================================
// II.   VOLUME (per-shape)
// ============================================================================

NS_INTERNAL

    // volume_impl (general fallback - declared only)
    //   helper: a deliberate static_assert primary. Shapes without a
    // closed-form volume must use volume_divergence with an explicit
    // boundary surface.
    template<typename _Shape,
             typename = void>
    struct volume_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "volume: no closed-form volume available. "
                      "Provide closed_form_volume() on the shape, "
                      "or call volume_divergence() with an explicit "
                      "boundary surface.");
    };

    // volume_impl (closed-form specialisation)
    template<typename _Shape>
    struct volume_impl<_Shape, std::enable_if_t<
        has_closed_form_volume<_Shape>::value
    >>
    {
        using value_type = typename _Shape::value_type;

        static value_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_volume();
        }
    };

NS_END  // internal

// volume
//   computes the volume of a 3D shape. Closed-form only; for
// arbitrary shapes use volume_divergence below.
template<typename _Shape>
typename _Shape::value_type
volume
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "volume: _Shape must satisfy the shape interface.");
    static_assert(_Shape::is_3d,
                  "volume: 3D shapes only (use area() for 2D).");

    return internal::volume_impl<_Shape>::compute(_s);
}

// volume_divergence
//   numerically computes the volume enclosed by a closed parametric
// surface via the divergence theorem:
//   V = (1/3) ∮ r · n dA
// where r is the position vector and n dA is the outward area
// element. We integrate (1/3)(x·(r_u × r_v)) over the parameter
// domain, with r_u and r_v from central differences. The user must
// ensure the surface is closed and consistently outward-oriented;
// the returned value is the absolute value of the signed integral.
template<typename _Surface>
typename _Surface::value_type
volume_divergence
(
    const _Surface& _boundary,
    std::size_t     _nu = 32,
    std::size_t     _nv = 32
) noexcept
{
    static_assert(is_surface<_Surface>::value,
                  "volume_divergence: _Surface must satisfy the "
                  "surface interface.");
    static_assert(_Surface::dimension == 3,
                  "volume_divergence: 3D surfaces only.");

    using value_type = typename _Surface::value_type;
    using sys        = typename _Surface::coordinate_system;

    value_type u_min;
    value_type u_max;
    value_type v_min;
    value_type v_max;
    value_type eps_u;
    value_type eps_v;
    value_type result;

    u_min = _boundary.parameter_min_u();
    u_max = _boundary.parameter_max_u();
    v_min = _boundary.parameter_min_v();
    v_max = _boundary.parameter_max_v();

    eps_u = (u_max - u_min) /
            static_cast<value_type>(_nu * 64);
    eps_v = (v_max - v_min) /
            static_cast<value_type>(_nv * 64);

    // integrand: r · (r_u × r_v) / 3, evaluated in Cartesian
    auto integrand =
        [&_boundary, eps_u, eps_v]
        (value_type _u, value_type _v) -> value_type
        {
            auto p       = sys::to_cartesian(_boundary.at(_u, _v));
            auto p_u_fwd = sys::to_cartesian(
                _boundary.at(_u + eps_u, _v));
            auto p_u_bck = sys::to_cartesian(
                _boundary.at(_u - eps_u, _v));
            auto p_v_fwd = sys::to_cartesian(
                _boundary.at(_u, _v + eps_v));
            auto p_v_bck = sys::to_cartesian(
                _boundary.at(_u, _v - eps_v));

            value_type two_eu = static_cast<value_type>(2) * eps_u;
            value_type two_ev = static_cast<value_type>(2) * eps_v;

            value_type rux = (p_u_fwd[0] - p_u_bck[0]) / two_eu;
            value_type ruy = (p_u_fwd[1] - p_u_bck[1]) / two_eu;
            value_type ruz = (p_u_fwd[2] - p_u_bck[2]) / two_eu;
            value_type rvx = (p_v_fwd[0] - p_v_bck[0]) / two_ev;
            value_type rvy = (p_v_fwd[1] - p_v_bck[1]) / two_ev;
            value_type rvz = (p_v_fwd[2] - p_v_bck[2]) / two_ev;

            // cross product r_u × r_v
            value_type cx = ruy * rvz - ruz * rvy;
            value_type cy = ruz * rvx - rux * rvz;
            value_type cz = rux * rvy - ruy * rvx;

            // r · (r_u × r_v) / 3
            return ( p[0] * cx + p[1] * cy + p[2] * cz ) /
                   static_cast<value_type>(3);
        };

    // outer Simpson in u of inner Simpson in v
    auto outer = [&integrand, v_min, v_max, _nv]
                 (value_type _u) -> value_type
    {
        auto inner = [&integrand, _u]
                     (value_type _v) -> value_type
        {
            return integrand(_u, _v);
        };

        return integrate_simpson<decltype(inner), value_type>(
            inner, v_min, v_max, _nv);
    };

    result = integrate_simpson<decltype(outer), value_type>(
        outer, u_min, u_max, _nu);

    // report absolute volume (winding orientation is the user's call)
    if (result < static_cast<value_type>(0))
    {
        result = -result;
    }

    return result;
}


// ============================================================================
// III.  SURFACE AREA (per-shape)
// ============================================================================

NS_INTERNAL

    // surface_area_impl (general fallback - declared only)
    template<typename _Shape,
             typename = void>
    struct surface_area_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "surface_area: no closed-form surface area "
                      "available. Provide "
                      "closed_form_surface_area() on the shape, or "
                      "sum patch_area() over the boundary surfaces.");
    };

    // surface_area_impl (closed-form specialisation)
    template<typename _Shape>
    struct surface_area_impl<_Shape, std::enable_if_t<
        has_closed_form_surface_area<_Shape>::value
    >>
    {
        using value_type = typename _Shape::value_type;

        static value_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_surface_area();
        }
    };

NS_END  // internal

// surface_area
//   computes the bounding surface area of a 3D shape. Closed-form
// only; for free-form shapes sum patch_area() over the boundary.
template<typename _Shape>
typename _Shape::value_type
surface_area
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "surface_area: _Shape must satisfy the shape "
                  "interface.");
    static_assert(_Shape::is_3d,
                  "surface_area: 3D shapes only (use perimeter() "
                  "for 2D).");

    return internal::surface_area_impl<_Shape>::compute(_s);
}


// ============================================================================
// IV.   CENTROID (per-shape, 3D)
// ============================================================================

NS_INTERNAL

    // centroid_3d_impl (general fallback - declared only)
    template<typename _Shape,
             typename = void>
    struct centroid_3d_impl
    {
        static_assert(sizeof(_Shape) == 0,
                      "centroid: no closed_form_centroid() on this "
                      "shape and no general numerical fallback. "
                      "Define closed_form_centroid().");
    };

    // centroid_3d_impl (closed-form specialisation)
    template<typename _Shape>
    struct centroid_3d_impl<_Shape, std::enable_if_t<
        has_closed_form_centroid<_Shape>::value
    >>
    {
        using point_type = typename _Shape::point_type;

        static point_type
        compute
        (
            const _Shape& _s
        ) noexcept
        {
            return _s.closed_form_centroid();
        }
    };

NS_END  // internal

// centroid
//   geometric centroid of a 3D shape. Closed-form only.
//
// (The 2D centroid for is_2d shapes lives in measure_2d.hpp; this
// overload is constrained via static_assert to is_3d, so the two
// don't collide.)
template<typename _Shape>
std::enable_if_t<_Shape::is_3d,
                 typename _Shape::point_type>
centroid_3d
(
    const _Shape& _s
) noexcept
{
    static_assert(is_shape<_Shape>::value,
                  "centroid_3d: _Shape must satisfy the shape "
                  "interface.");

    return internal::centroid_3d_impl<_Shape>::compute(_s);
}


// ============================================================================
// V.    BOUNDING BOXES
// ============================================================================
// All AABBs are reported in Cartesian space. For non-Cartesian
// shapes, the bounding box is the AABB of the shape's Cartesian
// image.

// bounding_box_of_surface
//   approximates the AABB of a surface patch by sampling
// (_nu + 1) × (_nv + 1) points over the parameter grid.
template<typename _Surface>
aabb<3, typename _Surface::value_type>
bounding_box_of_surface
(
    const _Surface& _s,
    std::size_t     _nu = 16,
    std::size_t     _nv = 16
) noexcept
{
    static_assert(is_surface<_Surface>::value,
                  "bounding_box_of_surface: _Surface must satisfy "
                  "the surface interface.");
    static_assert(_Surface::dimension == 3,
                  "bounding_box_of_surface: 3D surfaces only.");

    using value_type = typename _Surface::value_type;
    using sys        = typename _Surface::coordinate_system;

    value_type           u_min;
    value_type           u_max;
    value_type           v_min;
    value_type           v_max;
    value_type           du;
    value_type           dv;
    aabb<3, value_type>  box;

    u_min = _s.parameter_min_u();
    u_max = _s.parameter_max_u();
    v_min = _s.parameter_min_v();
    v_max = _s.parameter_max_v();
    du    = (u_max - u_min) / static_cast<value_type>(_nu);
    dv    = (v_max - v_min) / static_cast<value_type>(_nv);
    box   = aabb<3, value_type>::empty();

    // grid of (_nu + 1) × (_nv + 1) sample points
    for (std::size_t i = 0; i <= _nu; ++i)
    {
        value_type u = u_min +
            static_cast<value_type>(i) * du;

        for (std::size_t j = 0; j <= _nv; ++j)
        {
            value_type v = v_min +
                static_cast<value_type>(j) * dv;

            auto p_cart = sys::to_cartesian(_s.at(u, v));

            box.include(p_cart);
        }
    }

    return box;
}

// bounding_box_of_sphere
//   exact AABB of a sphere: centre ± radius on each axis.
template<typename _System>
aabb<3, typename _System::value_type>
bounding_box_of_sphere
(
    const sphere<_System>& _sph
) noexcept
{
    using value_type = typename _System::value_type;
    using point_3    = std::array<value_type, 3>;

    auto    cc = _System::to_cartesian(_sph.m_center);
    point_3 lo;
    point_3 hi;

    lo[0] = cc[0] - _sph.m_radius;
    lo[1] = cc[1] - _sph.m_radius;
    lo[2] = cc[2] - _sph.m_radius;
    hi[0] = cc[0] + _sph.m_radius;
    hi[1] = cc[1] + _sph.m_radius;
    hi[2] = cc[2] + _sph.m_radius;

    return aabb<3, value_type>{lo, hi};
}

// bounding_box_of_box
//   exact AABB of an axis-aligned box.
template<typename _System>
aabb<3, typename _System::value_type>
bounding_box_of_box
(
    const box<_System>& _b
) noexcept
{
    using value_type = typename _System::value_type;
    using point_3    = std::array<value_type, 3>;

    auto    cc = _System::to_cartesian(_b.m_corner);
    point_3 lo;
    point_3 hi;

    lo[0] = cc[0];
    lo[1] = cc[1];
    lo[2] = cc[2];
    hi[0] = cc[0] + _b.m_width;
    hi[1] = cc[1] + _b.m_height;
    hi[2] = cc[2] + _b.m_depth;

    return aabb<3, value_type>{lo, hi};
}

// bounding_box_of_cylinder
//   exact AABB of a +z-axis cylinder.
template<typename _System>
aabb<3, typename _System::value_type>
bounding_box_of_cylinder
(
    const cylinder<_System>& _cyl
) noexcept
{
    using value_type = typename _System::value_type;
    using point_3    = std::array<value_type, 3>;

    auto    bc = _System::to_cartesian(_cyl.m_base);
    point_3 lo;
    point_3 hi;

    lo[0] = bc[0] - _cyl.m_radius;
    lo[1] = bc[1] - _cyl.m_radius;
    lo[2] = bc[2];
    hi[0] = bc[0] + _cyl.m_radius;
    hi[1] = bc[1] + _cyl.m_radius;
    hi[2] = bc[2] + _cyl.m_height;

    return aabb<3, value_type>{lo, hi};
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_MEASURE_3D_
