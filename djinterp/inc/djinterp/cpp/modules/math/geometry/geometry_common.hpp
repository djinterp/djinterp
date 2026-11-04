/*******************************************************************************
* djinterp [cpp/math]                                        geometry_common.hpp
*
* Foundational geometry infrastructure shared by 2D and 3D modules.
*   Provides the structural interface documentation for edge and shape
* types, SFINAE detection traits, coordinate-system-aware lerp/distance
* helpers, axis-aligned bounding boxes, and composite Simpson's rule for
* numerical integration. Nothing in this header is dimension-specific;
* both shape_2d / measure_2d and future 3D modules build on it.
*
* STRUCTURAL EDGE INTERFACE (compile-time detectable):
*   - coordinate_system, value_type, point_type
*   - static std::size_t dimension
*   - static bool is_edge   = true
*   - static bool is_curved
*   - static bool is_closed
*   - value_type parameter_min() const
*   - value_type parameter_max() const
*   - point_type at(value_type _t) const
*   - point_type start() const
*   - point_type end() const
*   - (optional) value_type closed_form_length() const
*
* STRUCTURAL SHAPE INTERFACE (compile-time detectable):
*   - coordinate_system, value_type, point_type
*   - static std::size_t dimension
*   - static bool is_shape = true
*   - static bool is_2d
*   - static bool is_3d
*   - bool contains(const point_type& _p) const
*   - (optional) value_type closed_form_perimeter() const
*   - (optional) value_type closed_form_area() const
*   - (optional) value_type closed_form_volume() const           [3D]
*   - (optional) point_type closed_form_centroid() const
*
* LEVELS:
*   Everything compiles from C++98. A point is a linalg::vector (written
* {{x, y, z}}), so is a bounding box's corner; the traits detect by sizeof
* at every level, so they answer the same everywhere. pi_of<T>() and its
* kin are the constants at every level; pi_v<T> and its kin, variable
* templates, from C++14. From C++11, the Cartesian helpers also take the
* std::array points the geometry used before C++98 was its floor.
*
*
* path:      /inc/djinterp/cpp/modules/math/geometry/geometry_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DETECTION TRAITS
    ----------------
    1.  The structural flags
    2.  The closed forms
2.  COORDINATE-SYSTEM HELPERS
    -------------------------
    1.  cartesian_lerp, cartesian_distance
    2.  Cartesian products
3.  BOUNDING BOXES
    --------------
    1.  aabb
4.  NUMERICAL INTEGRATION
    ---------------------
5.  CONSTANTS
    ---------
*/

#ifndef DJINTERP_MATH_GEOMETRY_GEOMETRY_COMMON_HPP
#define DJINTERP_MATH_GEOMETRY_GEOMETRY_COMMON_HPP 1

// std
#include <cstddef>                                // std::size_t
#include <limits>                                 // std::numeric_limits
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../meta/trait_detect.hpp"         // D_TYPE_TRAIT_HAS_STATIC_
                                                  // MEMBER
#include "../coordinate/coordinate.hpp"           // the coordinate systems
#include "../linear_algebra/vector.hpp"           // linalg::vector
// re_std
#include "../../../../../re_std/cpp/type_traits/integral_constant.hpp"  // integral_
                                                                        // constant
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                              // std::array
#endif


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  DETECTION TRAITS
//==============================================================================
// Each detects by sizeof -- an expression's substitution succeeds or the
// overload drops (expression SFINAE, which GCC and Clang apply at every
// level) -- so a trait has one answer at every level.


// 1.1    The structural flags
//------------------------------------------------------------------------------
NS_INTERNAL

    D_TYPE_TRAIT_HAS_STATIC_MEMBER(geometry_names_is_edge, is_edge)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(geometry_names_is_shape, is_shape)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(geometry_names_is_surface, is_surface)

    // geometry_flag_is_edge, _is_shape, _is_surface
    //   trait: the static flag's value where the type names one, false
    // otherwise.
    template<typename Type,
             bool     Named = geometry_names_is_edge<Type>::value>
    struct geometry_flag_is_edge : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct geometry_flag_is_edge<Type, true>
        : re_std::integral_constant<bool, Type::is_edge>
    {};

    template<typename Type,
             bool     Named = geometry_names_is_shape<Type>::value>
    struct geometry_flag_is_shape : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct geometry_flag_is_shape<Type, true>
        : re_std::integral_constant<bool, Type::is_shape>
    {};

    template<typename Type,
             bool     Named = geometry_names_is_surface<Type>::value>
    struct geometry_flag_is_surface : re_std::integral_constant<bool, false>
    {};

    template<typename Type>
    struct geometry_flag_is_surface<Type, true>
        : re_std::integral_constant<bool, Type::is_surface>
    {};

NS_END  // internal

// is_edge, is_shape, is_surface
//   trait: the type declares itself an edge, a shape, a surface.
template<typename Type>
struct is_edge : internal::geometry_flag_is_edge<Type>
{};

template<typename Type>
struct is_shape : internal::geometry_flag_is_shape<Type>
{};

template<typename Type>
struct is_surface : internal::geometry_flag_is_surface<Type>
{};

// 1.2    The closed forms
//------------------------------------------------------------------------------
NS_INTERNAL

    typedef char geometry_probe_yes;
    typedef char (&geometry_probe_no)[2];

    // geometry_probe_size
    //   struct: names a size, so sizeof of a call can stand in a signature.
    template<std::size_t Size>
    struct geometry_probe_size
    {};

    // has_closed_form_*_check
    //   trait: the const member function closed_form_*() exists and is
    // callable with no arguments (its result type unconstrained; the comma
    // lets a void result through sizeof).
    template<typename Type>
    struct has_closed_form_surface_area_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_surface_area(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

    template<typename Type>
    struct has_closed_form_length_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_length(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

    template<typename Type>
    struct has_closed_form_perimeter_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_perimeter(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

    template<typename Type>
    struct has_closed_form_area_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_area(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

    template<typename Type>
    struct has_closed_form_volume_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_volume(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

    template<typename Type>
    struct has_closed_form_centroid_check
    {
    private:
        template<typename U>
        static geometry_probe_yes test(geometry_probe_size<sizeof(
            (static_cast<const U*>(0)->closed_form_centroid(), 0))>*);

        template<typename U>
        static geometry_probe_no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(geometry_probe_yes))
        };
    };

NS_END  // internal

// has_closed_form_surface_area, _length, _perimeter, _area, _volume,
// _centroid
//   trait: the type offers the closed form.
template<typename Type>
struct has_closed_form_surface_area
    : re_std::integral_constant<bool,
          (internal::has_closed_form_surface_area_check<Type>::value != 0)>
{};

template<typename Type>
struct has_closed_form_length
    : re_std::integral_constant<bool,
          (internal::has_closed_form_length_check<Type>::value != 0)>
{};

template<typename Type>
struct has_closed_form_perimeter
    : re_std::integral_constant<bool,
          (internal::has_closed_form_perimeter_check<Type>::value != 0)>
{};

template<typename Type>
struct has_closed_form_area
    : re_std::integral_constant<bool,
          (internal::has_closed_form_area_check<Type>::value != 0)>
{};

template<typename Type>
struct has_closed_form_volume
    : re_std::integral_constant<bool,
          (internal::has_closed_form_volume_check<Type>::value != 0)>
{};

template<typename Type>
struct has_closed_form_centroid
    : re_std::integral_constant<bool,
          (internal::has_closed_form_centroid_check<Type>::value != 0)>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// the _v companions (C++14)
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_edge_v = is_edge<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_shape_v = is_shape<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_surface_v = is_surface<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_surface_area_v =
    has_closed_form_surface_area<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_length_v =
    has_closed_form_length<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_perimeter_v =
    has_closed_form_perimeter<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_area_v =
    has_closed_form_area<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_volume_v =
    has_closed_form_volume<Type>::value;
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool has_closed_form_centroid_v =
    has_closed_form_centroid<Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


//==============================================================================
// 2.  COORDINATE-SYSTEM HELPERS
//==============================================================================


// 2.1    cartesian_lerp, cartesian_distance
//------------------------------------------------------------------------------

// cartesian_lerp
//   function: the point a fraction _t of the way from _a to _b along the
// straight (Cartesian) line, a + t (b - a), in _a and _b's system.
template<typename System>
D_CONSTEXPR_CPP14 typename System::point_type
cartesian_lerp(
    const typename System::point_type& _a,
    const typename System::point_type& _b,
    typename System::value_type        _t
) D_NOEXCEPT
{
    typedef typename System::point_type point_type;

    const point_type ca = System::to_cartesian(_a);
    const point_type cb = System::to_cartesian(_b);
    point_type       p;

    for (std::size_t i = 0; i < System::dimension; ++i)
    {
        p[i] = ca[i] + (_t * (cb[i] - ca[i]));
    }

    return System::from_cartesian(p);
}

// cartesian_distance
//   function: the straight-line distance between two points of a system.
template<typename System>
D_CONSTEXPR_CPP14 typename System::value_type
cartesian_distance(
    const typename System::point_type& _a,
    const typename System::point_type& _b
) D_NOEXCEPT
{
    return System::to_cartesian(_b).distance(System::to_cartesian(_a));
}

// 2.2    Cartesian products
//------------------------------------------------------------------------------
// The vector's, under the names the geometry uses; from C++11 also on the
// std::array points the geometry used before C++98 was its floor (a
// template does not convert its arguments, so the overloads are spelled).

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
cartesian_dot(
    const linalg::vector<Type, N>& _a,
    const linalg::vector<Type, N>& _b
) D_NOEXCEPT
{
    return _a.dot(_b);
}

template<typename Type>
D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
cartesian_cross_3(
    const linalg::vector<Type, 3>& _a,
    const linalg::vector<Type, 3>& _b
) D_NOEXCEPT
{
    return _a.cross(_b);
}

template<typename Type>
D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
cartesian_sub_3(
    const linalg::vector<Type, 3>& _a,
    const linalg::vector<Type, 3>& _b
) D_NOEXCEPT
{
    return _a.minus(_b);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
cartesian_dot(
    const std::array<Type, N>& _a,
    const std::array<Type, N>& _b
) D_NOEXCEPT
{
    return linalg::vector<Type, N>(_a).dot(linalg::vector<Type, N>(_b));
}

template<typename Type>
D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
cartesian_cross_3(
    const std::array<Type, 3>& _a,
    const std::array<Type, 3>& _b
) D_NOEXCEPT
{
    return linalg::vector<Type, 3>(_a).cross(linalg::vector<Type, 3>(_b));
}

template<typename Type>
D_CONSTEXPR_CPP14 linalg::vector<Type, 3>
cartesian_sub_3(
    const std::array<Type, 3>& _a,
    const std::array<Type, 3>& _b
) D_NOEXCEPT
{
    return linalg::vector<Type, 3>(_a).minus(linalg::vector<Type, 3>(_b));
}
#endif


//==============================================================================
// 3.  BOUNDING BOXES
//==============================================================================


// 3.1    aabb
//------------------------------------------------------------------------------
// 3.1.1
// aabb
//   struct: an axis-aligned bounding box, its corners m_min and m_max.
template<std::size_t Dim,
         typename    ValueType = double>
struct aabb
{
    typedef ValueType                       value_type;
    typedef linalg::vector<value_type, Dim> point_type;

    static D_CONSTEXPR_VAR std::size_t dimension = Dim;

    point_type m_min;
    point_type m_max;

    D_CONSTEXPR
    aabb() D_NOEXCEPT
        : m_min(),
          m_max()
    {}

    D_CONSTEXPR
    aabb(
        const point_type& _min_corner,
        const point_type& _max_corner
    ) D_NOEXCEPT
        : m_min(_min_corner),
          m_max(_max_corner)
    {}

    // extent
    //   the box's length along _axis.
    D_CONSTEXPR value_type
    extent(std::size_t _axis) const D_NOEXCEPT
    {
        return m_max[_axis] - m_min[_axis];
    }

    // center
    //   the midpoint of the corners.
    D_CONSTEXPR_CPP14 point_type
    center() const D_NOEXCEPT
    {
        point_type c;

        for (std::size_t i = 0; i < Dim; ++i)
        {
            c[i] = (m_min[i] + m_max[i]) / static_cast<value_type>(2);
        }

        return c;
    }

    // contains
    //   whether _p lies in the box, its faces included.
    D_CONSTEXPR_CPP14 bool
    contains(const point_type& _p) const D_NOEXCEPT
    {
        for (std::size_t i = 0; i < Dim; ++i)
        {
            if ( (_p[i] < m_min[i]) ||
                 (_p[i] > m_max[i]) )
            {
                return false;
            }
        }

        return true;
    }

    // include
    //   grows the box to hold _p.
    D_CONSTEXPR_CPP14 void
    include(const point_type& _p) D_NOEXCEPT
    {
        for (std::size_t i = 0; i < Dim; ++i)
        {
            if (_p[i] < m_min[i])
            {
                m_min[i] = _p[i];
            }

            if (_p[i] > m_max[i])
            {
                m_max[i] = _p[i];
            }
        }

        return;
    }

    // empty
    //   the empty box (min +inf, max -inf), which include grows from.
    static aabb
    empty() D_NOEXCEPT
    {
        return aabb(point_type::filled(
                        std::numeric_limits<value_type>::infinity()),
                    point_type::filled(
                        -std::numeric_limits<value_type>::infinity()));
    }

    // is_empty
    //   whether some axis has min above max.
    D_CONSTEXPR_CPP14 bool
    is_empty() const D_NOEXCEPT
    {
        for (std::size_t i = 0; i < Dim; ++i)
        {
            if (m_min[i] > m_max[i])
            {
                return true;
            }
        }

        return false;
    }
};

template<std::size_t Dim, typename ValueType>
D_CONSTEXPR_VAR std::size_t aabb<Dim, ValueType>::dimension;


//==============================================================================
// 4.  NUMERICAL INTEGRATION
//==============================================================================


NS_INTERNAL

    // simpson_composite
    //   function: composite Simpson's rule over [_a, _b] with _n intervals
    // (at least 2, rounded up to even).
    template<typename Func,
             typename ValueType>
    ValueType
    simpson_composite(
        Func        _f,
        ValueType   _a,
        ValueType   _b,
        std::size_t _n
    )
    {
        const std::size_t n     = (_n < 2) ? 2 : (_n + (_n % 2));
        const ValueType   h     = (_b - _a) / static_cast<ValueType>(n);
        ValueType         total = _f(_a) + _f(_b);

        // the interior points, weighted 4 and 2 in turn
        for (std::size_t i = 1; i < n; ++i)
        {
            const ValueType t = _a + (static_cast<ValueType>(i) * h);
            const ValueType w = ((i % 2) == 0) ? static_cast<ValueType>(2)
                                               : static_cast<ValueType>(4);

            total += w * _f(t);
        }

        return total * h / static_cast<ValueType>(3);
    }

NS_END  // internal

// integrate_simpson
//   function: the integral of _f over [_a, _b] by composite Simpson's rule
// with _n intervals (default 64).
template<typename Func,
         typename ValueType>
ValueType
integrate_simpson(
    Func        _f,
    ValueType   _a,
    ValueType   _b,
    std::size_t _n = 64
)
{
    return internal::simpson_composite(_f, _a, _b, _n);
}


//==============================================================================
// 5.  CONSTANTS
//==============================================================================


// pi_of, two_pi_of, half_pi_of
//   function: π, 2π and π/2 in Value, at every level.
template<typename Value>
D_CONSTEXPR Value
pi_of() D_NOEXCEPT
{
    return static_cast<Value>(3.14159265358979323846L);
}

template<typename Value>
D_CONSTEXPR Value
two_pi_of() D_NOEXCEPT
{
    return static_cast<Value>(2) * pi_of<Value>();
}

template<typename Value>
D_CONSTEXPR Value
half_pi_of() D_NOEXCEPT
{
    return pi_of<Value>() / static_cast<Value>(2);
}

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// pi_v, two_pi_v, half_pi_v
//   variable template: the same, as variable templates (C++14).
template<typename Value>
D_INLINE_VAR D_CONSTEXPR_VAR Value pi_v = pi_of<Value>();

template<typename Value>
D_INLINE_VAR D_CONSTEXPR_VAR Value two_pi_v = two_pi_of<Value>();

template<typename Value>
D_INLINE_VAR D_CONSTEXPR_VAR Value half_pi_v = half_pi_of<Value>();
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_GEOMETRY_GEOMETRY_COMMON_HPP
