/******************************************************************************
* djinterp [math]                                                     ray.hpp
*
* A ray (origin + direction) in Cartesian space, plus the intersection tests
* used for picking and containment queries.
*   The ray is a half-line: a point on it is m_origin + t * m_direction for
* t >= 0, and the direction need not be normalised. Two predicates are
* provided: the slab test against an axis-aligned box (either an aabb or a raw
* pair of corners) and Moller-Trumbore against a triangle. The triangle test
* forwards to the same internal::moller_trumbore kernel the triangle_mesh
* containment test uses, so the two never drift apart.
*
* CARTESIAN ONLY:
*   like the aabb and the mesh intersection kernel, a ray is expressed in
* Cartesian coordinates. A locus in another coordinate system is intersected
* by mapping it through its system's to_cartesian first.
*
* path:      /inc/djinterp/math/geometry/ray.hpp
* link:      TBA
* author(s): TBA                                           created: 2026.06.18
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_RAY_
#define DJINTERP_MATH_GEOMETRY_RAY_ 1

// std
#include <cstddef>
#include <array>
#include <limits>
// djinterp [math]
#include "../../djinterp.hpp"
#include "../coordinate/coordinate.hpp"
#include "./geometry_common.hpp"   // aabb, internal cartesian helpers
#include "./solid.hpp"             // internal::moller_trumbore


NS_DJINTERP
NS_MATH

// ===========================================================================
// I.    RAY
// ===========================================================================

// ray
//   struct: a half-line from m_origin in the (not necessarily unit) direction
// m_direction, expressed in Cartesian coordinates. A point on the ray is
// m_origin + t * m_direction for t >= 0.
template<typename _T = double>
struct ray
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _T;
    using point_type = std::array<_T, 3>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension = 3;

    // ---- data ---------------------------------------------------------------

    point_type m_origin;
    point_type m_direction;

    // ---- construction -------------------------------------------------------

    constexpr ray() noexcept
        : m_origin{},
          m_direction{}
    {
    }

    constexpr ray(
        const point_type& _origin,
        const point_type& _direction
    ) noexcept
        : m_origin(_origin),
          m_direction(_direction)
    {
    }

    // ---- evaluation ---------------------------------------------------------

    // at
    //   the point at parameter _t along the ray.
    D_NODISCARD D_CONSTEXPR point_type
    at(
        _T _t
    ) const noexcept
    {
        point_type p{};

        for (std::size_t i = 0; i < 3; ++i)
        {
            p[i] = m_origin[i] + m_direction[i] * _t;
        }

        return p;
    }
};


// ===========================================================================
// II.   RAY / AXIS-ALIGNED BOX
// ===========================================================================

// intersect_aabb
//   slab test of _ray against the axis-aligned box [_box_min, _box_max]. On a
// hit, writes the entry and exit parameters to _t_near / _t_far and returns
// true. A hit whose exit lies behind the origin (_t_far < 0) is rejected.
template<typename _T>
D_NODISCARD D_CONSTEXPR bool
intersect_aabb(
    const ray<_T>&           _ray,
    const std::array<_T, 3>& _box_min,
    const std::array<_T, 3>& _box_max,
    _T&                      _t_near,
    _T&                      _t_far
) noexcept
{
    _T t_min = -std::numeric_limits<_T>::infinity();
    _T t_max =  std::numeric_limits<_T>::infinity();

    for (std::size_t i = 0; i < 3; ++i)
    {
        const _T o = _ray.m_origin[i];
        const _T d = _ray.m_direction[i];

        // ray parallel to this slab: reject if the origin is outside it
        if ( (d > -static_cast<_T>(1e-12)) &&
             (d <  static_cast<_T>(1e-12)) )
        {
            if ( (o < _box_min[i]) ||
                 (o > _box_max[i]) )
            {
                return false;
            }

            continue;
        }

        const _T inv = static_cast<_T>(1) / d;
        _T       t1  = (_box_min[i] - o) * inv;
        _T       t2  = (_box_max[i] - o) * inv;

        // order so t1 is the near plane
        if (t1 > t2)
        {
            const _T tmp = t1;
            t1 = t2;
            t2 = tmp;
        }

        if (t1 > t_min)
        {
            t_min = t1;
        }

        if (t2 < t_max)
        {
            t_max = t2;
        }

        if (t_min > t_max)
        {
            return false;
        }
    }

    // box entirely behind the ray origin
    if (t_max < static_cast<_T>(0))
    {
        return false;
    }

    _t_near = t_min;
    _t_far  = t_max;

    return true;
}

// intersect_aabb
//   the same slab test taking an aabb directly. The box is Cartesian, matching
// the ray.
template<typename _T>
D_NODISCARD D_CONSTEXPR bool
intersect_aabb(
    const ray<_T>&     _ray,
    const aabb<3, _T>& _box,
    _T&                _t_near,
    _T&                _t_far
) noexcept
{
    return intersect_aabb(_ray, _box.m_min, _box.m_max, _t_near, _t_far);
}


// ===========================================================================
// III.  RAY / TRIANGLE
// ===========================================================================

// intersect_triangle
//   Moller-Trumbore intersection of _ray with the triangle (_v0, _v1, _v2).
// On a forward hit (t > _epsilon) writes the hit parameter to _t_out and
// returns true. Forwards to the shared internal kernel, so it stays in step
// with triangle_mesh::contains.
template<typename _T>
D_NODISCARD bool
intersect_triangle(
    const ray<_T>&           _ray,
    const std::array<_T, 3>& _v0,
    const std::array<_T, 3>& _v1,
    const std::array<_T, 3>& _v2,
    _T&                      _t_out,
    _T                       _epsilon = static_cast<_T>(1e-9)
) noexcept
{
    return internal::moller_trumbore<_T>(
        _ray.m_origin,
        _ray.m_direction,
        _v0,
        _v1,
        _v2,
        _t_out,
        _epsilon);
}

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_RAY_
