/******************************************************************************
* djinterp [math]                                             cartesian.hpp
*
* Compile-time N-dimensional Cartesian coordinate system.
*   Provides the Cartesian coordinate system type, point representation,
* scale factors, and differential geometry infrastructure. Cartesian
* coordinates are the simplest orthogonal system: all scale factors are 1,
* the Jacobian is 1, and the basis vectors are the standard unit vectors.
*
* STRUCTURAL INTERFACE (for coordinate traits):
*   - value_type, point_type
*   - static constexpr dimension
*   - static constexpr bool is_cartesian  = true
*   - static constexpr bool is_polar      = false
*   - static constexpr bool is_cylindrical = false
*   - static constexpr bool is_spherical  = false
*   - static constexpr bool is_orthogonal = true
*   - scale_factors(point), jacobian(point)
*   - to_cartesian(point), from_cartesian(point)
*
* AXIS NAMING:
*   1D: x
*   2D: x, y
*   3D: x, y, z
*   ND: x₀, x₁, ..., x_{N-1}
*
* path:      /inc/djinterp/math/coordinate/cartesian.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATH_CARTESIAN_
#define DJINTERP_MATH_CARTESIAN_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <type_traits>
#include "../djinterp.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    CARTESIAN COORDINATE SYSTEM
// ============================================================================

// cartesian
//   struct: N-dimensional Cartesian coordinate system.
// All scale factors are unity. The coordinate-to-Cartesian transform
// is the identity. This is the reference frame to which all other
// coordinate systems convert.
template<std::size_t _Dimension = 3,
         typename    _ValueType = double>
struct cartesian
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _ValueType;
    using point_type = std::array<value_type, _Dimension>;

    // ---- static constants ---------------------------------------------------

    static constexpr std::size_t dimension      = _Dimension;
    static constexpr bool        is_cartesian   = true;
    static constexpr bool        is_polar       = false;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical   = false;
    static constexpr bool        is_orthogonal  = true;

    static_assert(_Dimension > 0,
                  "cartesian: dimension must be at least 1.");

    // ---- coordinate access --------------------------------------------------

    // get
    //   returns the _I-th coordinate of a point.
    template<std::size_t _I>
    static constexpr value_type
    get
    (
        const point_type& _point
    ) noexcept
    {
        static_assert(_I < _Dimension,
                      "cartesian::get: index out of range.");

        return _point[_I];
    }

    // x
    //   returns the first coordinate.
    static constexpr value_type
    x
    (
        const point_type& _point
    ) noexcept
    {
        return _point[0];
    }

    // y
    //   returns the second coordinate (requires dimension >= 2).
    template<std::size_t _D = _Dimension,
             typename = std::enable_if_t<(_D >= 2)>>
    static constexpr value_type
    y
    (
        const point_type& _point
    ) noexcept
    {
        return _point[1];
    }

    // z
    //   returns the third coordinate (requires dimension >= 3).
    template<std::size_t _D = _Dimension,
             typename = std::enable_if_t<(_D >= 3)>>
    static constexpr value_type
    z
    (
        const point_type& _point
    ) noexcept
    {
        return _point[2];
    }


    // ---- point construction -------------------------------------------------

    // origin
    //   returns the origin point (all zeros).
    static constexpr point_type
    origin
    () noexcept
    {
        point_type result{};

        return result;
    }

    // unit
    //   returns the unit point along axis _I.
    template<std::size_t _I>
    static constexpr point_type
    unit
    () noexcept
    {
        static_assert(_I < _Dimension,
                      "cartesian::unit: index out of range.");

        point_type result{};
        result[_I] = static_cast<value_type>(1);

        return result;
    }

    // make_point (2D)
    //   constructs a 2D point.
    template<std::size_t _D = _Dimension,
             typename = std::enable_if_t<(_D == 2)>>
    static constexpr point_type
    make_point
    (
        value_type _x,
        value_type _y
    ) noexcept
    {
        return {{ _x, _y }};
    }

    // make_point (3D)
    //   constructs a 3D point.
    template<std::size_t _D = _Dimension,
             typename = std::enable_if_t<(_D == 3)>>
    static constexpr point_type
    make_point
    (
        value_type _x,
        value_type _y,
        value_type _z
    ) noexcept
    {
        return {{ _x, _y, _z }};
    }


    // ---- scale factors ------------------------------------------------------

    // scale_factors
    //   returns the scale factors at a given point.
    // In Cartesian coordinates, all scale factors are 1.
    static constexpr std::array<value_type, _Dimension>
    scale_factors
    (
        const point_type&
    ) noexcept
    {
        std::array<value_type, _Dimension> h{};

        for (std::size_t i = 0; i < _Dimension; ++i)
        {
            h[i] = static_cast<value_type>(1);
        }

        return h;
    }

    // jacobian
    //   returns the Jacobian determinant at a given point.
    // In Cartesian coordinates, the Jacobian is always 1.
    static constexpr value_type
    jacobian
    (
        const point_type&
    ) noexcept
    {
        return static_cast<value_type>(1);
    }

    // volume_element
    //   returns the volume element scale factor dV/(dx₁ dx₂ ... dxₙ).
    // In Cartesian coordinates, this is always 1.
    static constexpr value_type
    volume_element
    (
        const point_type&
    ) noexcept
    {
        return static_cast<value_type>(1);
    }


    // ---- coordinate conversion (identity) -----------------------------------

    // to_cartesian
    //   converts a Cartesian point to Cartesian (identity).
    static constexpr point_type
    to_cartesian
    (
        const point_type& _point
    ) noexcept
    {
        return _point;
    }

    // from_cartesian
    //   converts a Cartesian point from Cartesian (identity).
    static constexpr point_type
    from_cartesian
    (
        const point_type& _point
    ) noexcept
    {
        return _point;
    }


    // ---- metric operations --------------------------------------------------

    // distance_squared
    //   returns the squared Euclidean distance between two points.
    static constexpr value_type
    distance_squared
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        value_type sum = static_cast<value_type>(0);

        for (std::size_t i = 0; i < _Dimension; ++i)
        {
            value_type d = _a[i] - _b[i];
            sum += d * d;
        }

        return sum;
    }

    // distance
    //   returns the Euclidean distance between two points.
    static value_type
    distance
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        return std::sqrt(distance_squared(_a, _b));
    }

    // norm_squared
    //   returns the squared magnitude of a point/vector.
    static constexpr value_type
    norm_squared
    (
        const point_type& _point
    ) noexcept
    {
        value_type sum = static_cast<value_type>(0);

        for (std::size_t i = 0; i < _Dimension; ++i)
        {
            sum += _point[i] * _point[i];
        }

        return sum;
    }

    // norm
    //   returns the Euclidean magnitude of a point/vector.
    static value_type
    norm
    (
        const point_type& _point
    ) noexcept
    {
        return std::sqrt(norm_squared(_point));
    }

    // dot
    //   returns the dot product of two vectors.
    static constexpr value_type
    dot
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        value_type sum = static_cast<value_type>(0);

        for (std::size_t i = 0; i < _Dimension; ++i)
        {
            sum += _a[i] * _b[i];
        }

        return sum;
    }

    // cross (3D only)
    //   returns the cross product of two 3D vectors.
    template<std::size_t _D = _Dimension,
             typename = std::enable_if_t<(_D == 3)>>
    static constexpr point_type
    cross
    (
        const point_type& _a,
        const point_type& _b
    ) noexcept
    {
        return {{ _a[1] * _b[2] - _a[2] * _b[1],
                  _a[2] * _b[0] - _a[0] * _b[2],
                  _a[0] * _b[1] - _a[1] * _b[0] }};
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// cartesian_1d
//   type: 1-dimensional Cartesian coordinate system.
template<typename _T = double>
using cartesian_1d = cartesian<1, _T>;

// cartesian_2d
//   type: 2-dimensional Cartesian coordinate system.
template<typename _T = double>
using cartesian_2d = cartesian<2, _T>;

// cartesian_3d
//   type: 3-dimensional Cartesian coordinate system.
template<typename _T = double>
using cartesian_3d = cartesian<3, _T>;

// cartesian_4d
//   type: 4-dimensional Cartesian coordinate system.
template<typename _T = double>
using cartesian_4d = cartesian<4, _T>;

// cartesian_nd
//   type: alias for N-dimensional Cartesian (explicit).
template<std::size_t _N, typename _T = double>
using cartesian_nd = cartesian<_N, _T>;

// cartesian_point
//   type: point in N-dimensional Cartesian space.
template<std::size_t _N, typename _T = double>
using cartesian_point = typename cartesian<_N, _T>::point_type;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CARTESIAN_
