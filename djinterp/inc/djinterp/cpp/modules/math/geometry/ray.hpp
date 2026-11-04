/*******************************************************************************
* djinterp [cpp/math]                                                    ray.hpp
*
* A ray (origin + direction) in Cartesian space, plus the intersection tests
* used for picking and containment queries.
*   The ray is a half-line: a point on it is m_origin + t * m_direction for
* t >= 0, and the direction need not be normalised. Two predicates are
* provided: the slab test against an axis-aligned box (either an aabb or a raw
* pair of corners) and Moller-Trumbore against a triangle.
*
* CARTESIAN ONLY:
*   like the aabb and the mesh intersection kernel, a ray is expressed in
* Cartesian coordinates. A locus in another coordinate system is intersected
* by mapping it through its system's to_cartesian first.
*
* A FACE OVER THE C CORE:
*   Both tests are c/math/isect.h's, the formulas this header and solid.hpp
* wrote, in their order: the triangle test is the one the triangle mesh's
* containment test uses, so the two do not drift apart. An element type
* other than float, double and long double goes through double. A point is
* a linalg::vector<T, 3>; from C++11 a std::array corner or vertex converts
* where one is taken. Everything compiles from C++98; the tests are constant
* expressions from C++14.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/ray.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE RAY
    -------
    1.  ray
2.  INTERSECTIONS
    -------------
    1.  intersect_aabb
    2.  intersect_triangle
*/

#ifndef DJINTERP_MATH_GEOMETRY_RAY_HPP
#define DJINTERP_MATH_GEOMETRY_RAY_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../../c/modules/math/isect.h"     // the C core: d_isect_*
#include "../linear_algebra/linalg_common.hpp"    // linalg::internal::
                                                  // linalg_family_of
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./geometry_common.hpp"                  // aabb


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE RAY
//==============================================================================


// 1.1    ray
//------------------------------------------------------------------------------
// 1.1.1
// ray
//   struct: a half-line m_origin + t m_direction, t >= 0, in Cartesian
// space.
template<typename T = double>
struct ray
{
    typedef T                        value_type;
    typedef linalg::vector<T, 3>     point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = 3;

    point_type m_origin;
    point_type m_direction;

    D_CONSTEXPR
    ray() D_NOEXCEPT
        : m_origin(),
          m_direction()
    {}

    D_CONSTEXPR
    ray(
        const point_type& _origin,
        const point_type& _direction
    ) D_NOEXCEPT
        : m_origin(_origin),
          m_direction(_direction)
    {}

    // at
    //   the point m_origin + m_direction t.
    D_NODISCARD D_CONSTEXPR_CPP14 point_type
    at(T _t) const D_NOEXCEPT
    {
        return m_origin.plus(m_direction.scaled(_t));
    }
};

template<typename T>
D_CONSTEXPR_VAR std::size_t ray<T>::dimension;

NS_INTERNAL

    // ray_isect
    //   struct: the C core's tests for float, double and long double, and
    // the same through double for any other element type.
    template<typename T,
             int      Family = linalg::internal::linalg_family_of<T>::value>
    struct ray_isect
    {
        static D_CONSTEXPR_CPP14 bool
        aabb(const T* _o,
             const T* _d,
             const T* _lo,
             const T* _hi,
             T*       _t_near,
             T*       _t_far) D_NOEXCEPT
        {
            return d_isect_ray_aabb(_o, _d, _lo, _hi, _t_near, _t_far);
        }

        static D_CONSTEXPR_CPP14 bool
        triangle(const T* _o,
                 const T* _d,
                 const T* _v0,
                 const T* _v1,
                 const T* _v2,
                 T        _epsilon,
                 T*       _t_out) D_NOEXCEPT
        {
            return d_isect_ray_triangle(_o, _d, _v0, _v1, _v2, _epsilon,
                                        _t_out);
        }
    };

    template<typename T>
    struct ray_isect<T, 0>
    {
        static D_CONSTEXPR_CPP14 bool
        aabb(const T* _o,
             const T* _d,
             const T* _lo,
             const T* _hi,
             T*       _t_near,
             T*       _t_far) D_NOEXCEPT
        {
            double o[3]  = { 0.0, 0.0, 0.0 };
            double d[3]  = { 0.0, 0.0, 0.0 };
            double lo[3] = { 0.0, 0.0, 0.0 };
            double hi[3] = { 0.0, 0.0, 0.0 };
            double tn    = 0.0;
            double tf    = 0.0;

            for (std::size_t i = 0; i < 3; ++i)
            {
                o[i]  = static_cast<double>(_o[i]);
                d[i]  = static_cast<double>(_d[i]);
                lo[i] = static_cast<double>(_lo[i]);
                hi[i] = static_cast<double>(_hi[i]);
            }

            const bool hit = d_isect_d_ray_aabb(o, d, lo, hi, &tn, &tf);

            // the parameters only for a hit, as the families' kernel
            if (hit)
            {
                *_t_near = static_cast<T>(tn);
                *_t_far  = static_cast<T>(tf);
            }

            return hit;
        }

        static D_CONSTEXPR_CPP14 bool
        triangle(const T* _o,
                 const T* _d,
                 const T* _v0,
                 const T* _v1,
                 const T* _v2,
                 T        _epsilon,
                 T*       _t_out) D_NOEXCEPT
        {
            double o[3]  = { 0.0, 0.0, 0.0 };
            double d[3]  = { 0.0, 0.0, 0.0 };
            double v0[3] = { 0.0, 0.0, 0.0 };
            double v1[3] = { 0.0, 0.0, 0.0 };
            double v2[3] = { 0.0, 0.0, 0.0 };
            double t     = 0.0;

            for (std::size_t i = 0; i < 3; ++i)
            {
                o[i]  = static_cast<double>(_o[i]);
                d[i]  = static_cast<double>(_d[i]);
                v0[i] = static_cast<double>(_v0[i]);
                v1[i] = static_cast<double>(_v1[i]);
                v2[i] = static_cast<double>(_v2[i]);
            }

            const bool hit = d_isect_d_ray_triangle(
                o, d, v0, v1, v2, static_cast<double>(_epsilon), &t);

            // the parameter only for a hit, as the families' kernel
            if (hit)
            {
                *_t_out = static_cast<T>(t);
            }

            return hit;
        }
    };

NS_END  // internal


//==============================================================================
// 2.  INTERSECTIONS
//==============================================================================
// A corner or vertex is ray<T>::point_type, named through the ray so that T
// is deduced from the ray alone: a std::array then converts (C++11).


// 2.1    intersect_aabb
//------------------------------------------------------------------------------
// 2.1.1
// intersect_aabb
//   function: whether _ray meets the box [_box_min, _box_max] at a
// non-negative parameter, by the slab test; if so, _t_near and _t_far are
// where it enters and leaves (an origin inside enters behind it).
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 bool
intersect_aabb(
    const ray<T>&                       _ray,
    const typename ray<T>::point_type&  _box_min,
    const typename ray<T>::point_type&  _box_max,
    T&                                  _t_near,
    T&                                  _t_far
) D_NOEXCEPT
{
    return internal::ray_isect<T>::aabb(_ray.m_origin.data(),
                                        _ray.m_direction.data(),
                                        _box_min.data(),
                                        _box_max.data(),
                                        &_t_near,
                                        &_t_far);
}

// intersect_aabb
//   function: the same against an aabb.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 bool
intersect_aabb(
    const ray<T>&     _ray,
    const aabb<3, T>& _box,
    T&                _t_near,
    T&                _t_far
) D_NOEXCEPT
{
    return intersect_aabb(_ray, _box.m_min, _box.m_max, _t_near, _t_far);
}

// 2.2    intersect_triangle
//------------------------------------------------------------------------------
// 2.2.1
// intersect_triangle
//   function: whether _ray meets the triangle (_v0, _v1, _v2) at a parameter
// above _epsilon, by Moller-Trumbore; if so, _t_out is the parameter.
template<typename T>
D_NODISCARD D_CONSTEXPR_CPP14 bool
intersect_triangle(
    const ray<T>&                       _ray,
    const typename ray<T>::point_type&  _v0,
    const typename ray<T>::point_type&  _v1,
    const typename ray<T>::point_type&  _v2,
    T&                                  _t_out,
    T                                   _epsilon = static_cast<T>(1e-9)
) D_NOEXCEPT
{
    return internal::ray_isect<T>::triangle(_ray.m_origin.data(),
                                            _ray.m_direction.data(),
                                            _v0.data(),
                                            _v1.data(),
                                            _v2.data(),
                                            _epsilon,
                                            &_t_out);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_RAY_HPP
