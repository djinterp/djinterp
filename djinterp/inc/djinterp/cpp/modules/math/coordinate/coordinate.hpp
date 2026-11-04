/*******************************************************************************
* djinterp [cpp/math]                                             coordinate.hpp
*
* Unified coordinate system header.
*   This header includes all coordinate sub-modules (Cartesian, polar,
* cylindrical, spherical) and provides cross-system conversion dispatch,
* the system-agnostic point wrapper, and differential geometry utilities
* that work with any orthogonal coordinate system.
*
* SUB-MODULES:
*   cartesian.hpp   - N-dimensional Cartesian (x, y, z, ...)
*   polar.hpp       - 2D polar (r, θ)
*   cylindrical.hpp - 3D cylindrical (ρ, φ, z)
*   spherical.hpp   - 3D spherical (r, θ, φ)
*
* PROVIDED BY THIS HEADER:
*   coord_point<System>        - typed point in a coordinate system
*   coord_convert<From, To>    - generic cross-system conversion
*   coord_cast<To>(from_point) - free function conversion
*   differential<System>       - differential operator helpers
*
* STRUCTURAL REQUIREMENTS FOR COORDINATE SYSTEMS:
*   A coordinate system type must have:
*     - value_type, point_type, vector_type
*     - static dimension
*     - static bool is_cartesian, is_polar, is_cylindrical, is_spherical
*     - static bool is_orthogonal
*     - static to_cartesian(point_type) -> point_type
*     - static from_cartesian(point_type) -> point_type
*     - static scale_factors(point_type) -> vector_type
*     - static jacobian(point_type) -> value_type
*
* LEVELS:
*   Everything compiles from C++98: a point is a linalg::vector, the traits
* detect by sizeof below C++11, and a conversion is a constant expression
* where both systems' are. The _v variable templates are C++14's, and the
* point_2d / point_3d alias templates C++11's.
*
*
* path:      /inc/djinterp/cpp/modules/math/coordinate/coordinate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.10.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SYSTEM-AGNOSTIC POINT
    -------------------------
    1.  coord_point
2.  CONVERSION
    ----------
    1.  coord_convert
    2.  coord_cast
3.  DIFFERENTIAL GEOMETRY
    ---------------------
    1.  differential
4.  TRAITS
    ------
    1.  is_coord_system
    2.  are_same_system
    3.  are_compatible_systems
5.  POINT ALIASES
    -------------
*/

#ifndef DJINTERP_MATH_COORDINATE_COORDINATE_HPP
#define DJINTERP_MATH_COORDINATE_COORDINATE_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../linear_algebra/vector.hpp"           // linalg::vector
#include "./cartesian.hpp"                        // cartesian
#include "./cylindrical.hpp"                      // cylindrical
#include "./polar.hpp"                            // polar
#include "./spherical.hpp"                        // spherical
// re_std
#include "../../../../../re_std/cpp/type_traits/integral_constant.hpp"  // integral_
                                                                        // constant
#include "../../../../../re_std/cpp/type_traits/is_same.hpp"            // is_same


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE SYSTEM-AGNOSTIC POINT
//==============================================================================


// 1.1    coord_point
//------------------------------------------------------------------------------
// 1.1.1
// coord_point
//   struct: a point tagged with the coordinate system it is written in.
template<typename System>
struct coord_point
{
    typedef System                         system_type;
    typedef typename System::value_type    value_type;
    typedef typename System::point_type    point_type;
    typedef typename System::vector_type   vector_type;

    static D_CONSTEXPR_VAR std::size_t dimension = System::dimension;

    point_type data;

    D_CONSTEXPR
    coord_point() D_NOEXCEPT
        : data()
    {}

    D_CONSTEXPR explicit
    coord_point(const point_type& _p) D_NOEXCEPT
        : data(_p)
    {}

    D_CONSTEXPR value_type
    operator[](std::size_t _i) const D_NOEXCEPT
    {
        return data[_i];
    }

    D_CONSTEXPR_CPP14 value_type&
    operator[](std::size_t _i) D_NOEXCEPT
    {
        return data[_i];
    }

    D_CONSTEXPR const point_type&
    raw() const D_NOEXCEPT
    {
        return data;
    }

    D_CONSTEXPR_CPP14 point_type&
    raw() D_NOEXCEPT
    {
        return data;
    }

    // scale_factors, jacobian
    //   the system's, at this point.
    D_CONSTEXPR_CPP14 vector_type
    scale_factors() const D_NOEXCEPT
    {
        return System::scale_factors(data);
    }

    D_CONSTEXPR_CPP14 value_type
    jacobian() const D_NOEXCEPT
    {
        return System::jacobian(data);
    }

    // to_cartesian
    //   this point in the Cartesian system of its dimension.
    D_CONSTEXPR_CPP14 coord_point<cartesian<System::dimension, value_type> >
    to_cartesian() const D_NOEXCEPT
    {
        return coord_point<cartesian<System::dimension, value_type> >(
            System::to_cartesian(data));
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t coord_point<System>::dimension;


//==============================================================================
// 2.  CONVERSION
//==============================================================================


// 2.1    coord_convert
//------------------------------------------------------------------------------
NS_INTERNAL

    // convert_impl
    //   struct: From -> To through Cartesian coordinates, or directly where
    // a direct form exists (the identity; cylindrical <-> spherical).
    template<typename From,
             typename To,
             bool     SameSystem = re_std::is_same<From, To>::value>
    struct convert_impl
    {
        typedef typename From::point_type from_point;
        typedef typename To::point_type   to_point;

        static D_CONSTEXPR_CPP14 to_point
        convert(const from_point& _point) D_NOEXCEPT
        {
            return To::from_cartesian(From::to_cartesian(_point));
        }
    };

    template<typename System>
    struct convert_impl<System, System, true>
    {
        typedef typename System::point_type point_type;

        static D_CONSTEXPR point_type
        convert(const point_type& _point) D_NOEXCEPT
        {
            return _point;
        }
    };

    template<typename Type>
    struct convert_impl<cylindrical<Type>, spherical<Type>, false>
    {
        typedef typename cylindrical<Type>::point_type from_point;
        typedef typename spherical<Type>::point_type   to_point;

        static D_CONSTEXPR_CPP20 to_point
        convert(const from_point& _point) D_NOEXCEPT
        {
            return cylindrical<Type>::to_spherical(_point);
        }
    };

    template<typename Type>
    struct convert_impl<spherical<Type>, cylindrical<Type>, false>
    {
        typedef typename spherical<Type>::point_type   from_point;
        typedef typename cylindrical<Type>::point_type to_point;

        static D_CONSTEXPR_CPP20 to_point
        convert(const from_point& _point) D_NOEXCEPT
        {
            return spherical<Type>::to_cylindrical(_point);
        }
    };

NS_END  // internal

// 2.1.1
// coord_convert
//   struct: conversion between two coordinate systems of one dimension (or
// to and from Cartesian).
template<typename From,
         typename To>
struct coord_convert
{
    typedef From                      from_type;
    typedef To                        to_type;
    typedef typename From::point_type from_point;
    typedef typename To::point_type   to_point;

    D_STATIC_ASSERT(( (From::dimension == To::dimension) ||
                      (From::is_cartesian) ||
                      (To::is_cartesian) ),
                    "coord_convert: dimension mismatch between coordinate "
                    "systems.");

    static D_CONSTEXPR_CPP14 to_point
    convert(const from_point& _point) D_NOEXCEPT
    {
        return internal::convert_impl<From, To>::convert(_point);
    }

    static D_CONSTEXPR_CPP14 coord_point<To>
    convert(const coord_point<From>& _point) D_NOEXCEPT
    {
        return coord_point<To>(
            internal::convert_impl<From, To>::convert(_point.raw()));
    }
};

// 2.2    coord_cast
//------------------------------------------------------------------------------
// 2.2.1
// coord_cast
//   function: a point converted to the system To (From deduced from a
// coord_point; named for a raw point).
template<typename To,
         typename From>
D_CONSTEXPR_CPP14 coord_point<To>
coord_cast(const coord_point<From>& _point) D_NOEXCEPT
{
    return coord_convert<From, To>::convert(_point);
}

template<typename To,
         typename From>
D_CONSTEXPR_CPP14 typename To::point_type
coord_cast(const typename From::point_type& _point) D_NOEXCEPT
{
    return coord_convert<From, To>::convert(_point);
}


//==============================================================================
// 3.  DIFFERENTIAL GEOMETRY
//==============================================================================


// 3.1    differential
//------------------------------------------------------------------------------
// 3.1.1
// differential
//   struct: the line, surface and volume elements and the gradient,
// divergence and Laplacian factors of an orthogonal system, from its scale
// factors h and Jacobian J.
template<typename System>
struct differential
{
    typedef typename System::value_type  value_type;
    typedef typename System::point_type  point_type;
    typedef typename System::vector_type vector_type;

    static D_CONSTEXPR_VAR std::size_t dim = System::dimension;

    D_STATIC_ASSERT((System::is_orthogonal),
                    "differential: requires an orthogonal coordinate system.");

    // line_element_squared
    //   ds² = Σ (h_i dq_i)².
    static D_CONSTEXPR_CPP14 value_type
    line_element_squared(
        const point_type&  _point,
        const vector_type& _dq
    ) D_NOEXCEPT
    {
        return System::scale_factors(_point).hadamard(_dq).norm_squared();
    }

    // line_element
    //   ds, the root of line_element_squared.
    static D_CONSTEXPR_CPP14 value_type
    line_element(
        const point_type&  _point,
        const vector_type& _dq
    ) D_NOEXCEPT
    {
        return System::scale_factors(_point).hadamard(_dq).norm();
    }

    // volume_element
    //   dV / (dq_1 ... dq_n) = J.
    static D_CONSTEXPR_CPP14 value_type
    volume_element(const point_type& _point) D_NOEXCEPT
    {
        return System::jacobian(_point);
    }

    // surface_element
    //   the product of every scale factor but _omit_axis's.
    static D_CONSTEXPR_CPP14 value_type
    surface_element(
        const point_type& _point,
        std::size_t       _omit_axis
    ) D_NOEXCEPT
    {
        const vector_type h  = System::scale_factors(_point);
        value_type        dA = static_cast<value_type>(1);

        for (std::size_t i = 0; i < dim; ++i)
        {
            if (i != _omit_axis)
            {
                dA *= h[i];
            }
        }

        return dA;
    }

    // gradient_component
    //   (∇f)_axis = (∂f/∂q_axis) / h_axis; 0 where h_axis is.
    static D_CONSTEXPR_CPP14 value_type
    gradient_component(
        const point_type& _point,
        std::size_t       _axis,
        value_type        _partial_derivative
    ) D_NOEXCEPT
    {
        const vector_type h = System::scale_factors(_point);

        return (h[_axis] == static_cast<value_type>(0))
                   ? static_cast<value_type>(0)
                   : (_partial_derivative / h[_axis]);
    }

    // divergence_prefactor
    //   1 / J; 0 where J is.
    static D_CONSTEXPR_CPP14 value_type
    divergence_prefactor(const point_type& _point) D_NOEXCEPT
    {
        const value_type j = System::jacobian(_point);

        return (j == static_cast<value_type>(0))
                   ? static_cast<value_type>(0)
                   : (static_cast<value_type>(1) / j);
    }

    // laplacian_scale
    //   J / h_i² per axis; 0 where h_i is.
    static D_CONSTEXPR_CPP14 vector_type
    laplacian_scale(const point_type& _point) D_NOEXCEPT
    {
        const vector_type h = System::scale_factors(_point);
        const value_type  j = System::jacobian(_point);
        vector_type       scale;

        for (std::size_t i = 0; i < dim; ++i)
        {
            const value_type hi2 = h[i] * h[i];

            scale[i] = (hi2 == static_cast<value_type>(0))
                           ? static_cast<value_type>(0)
                           : (j / hi2);
        }

        return scale;
    }
};

template<typename System>
D_CONSTEXPR_VAR std::size_t differential<System>::dim;


//==============================================================================
// 4.  TRAITS
//==============================================================================


NS_INTERNAL

    // is_coord_system_check
    //   trait: true for a type with a static dimension and is_orthogonal, the
    // structural interface's two constants -- detected by sizeof, which every
    // level has: a probe whose defaults name them substitutes, or not.
    template<typename Type>
    struct is_coord_system_check
    {
    private:
        typedef char yes;
        typedef char (&no)[2];

        template<typename U,
                 std::size_t D = U::dimension,
                 bool        O = U::is_orthogonal>
        struct probe
        {};

        template<typename U>
        static yes test(probe<U>*);

        template<typename U>
        static no test(...);

    public:
        enum
        {
            value = (sizeof(test<Type>(0)) == sizeof(yes))
        };
    };

    // compatible_dimensions
    //   trait: the dimensions agree, asked only of two coordinate systems.
    template<typename A,
             typename B,
             bool     Both = ( is_coord_system_check<A>::value &&
                               is_coord_system_check<B>::value )>
    struct compatible_dimensions
    {
        enum
        {
            value = 0
        };
    };

    template<typename A,
             typename B>
    struct compatible_dimensions<A, B, true>
    {
        enum
        {
            value = (A::dimension == B::dimension) ? 1 : 0
        };
    };

NS_END  // internal

// 4.1    is_coord_system
//------------------------------------------------------------------------------
// 4.1.1
// is_coord_system
//   trait: Type has the coordinate system's structural constants.
template<typename Type>
struct is_coord_system
    : re_std::integral_constant<bool,
          (internal::is_coord_system_check<Type>::value != 0)>
{};

// 4.2    are_same_system
//------------------------------------------------------------------------------
// 4.2.1
// are_same_system
//   trait: A and B are the same system.
template<typename A,
         typename B>
struct are_same_system
    : re_std::integral_constant<bool, re_std::is_same<A, B>::value>
{};

// 4.3    are_compatible_systems
//------------------------------------------------------------------------------
// 4.3.1
// are_compatible_systems
//   trait: A and B are coordinate systems of one dimension.
template<typename A,
         typename B>
struct are_compatible_systems
    : re_std::integral_constant<bool,
          (internal::compatible_dimensions<A, B>::value != 0)>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_coord_system_v, are_same_system_v, are_compatible_systems_v
//   variable template: the traits' values (C++14).
template<typename Type>
D_INLINE_VAR D_CONSTEXPR_VAR bool is_coord_system_v =
    is_coord_system<Type>::value;

template<typename A,
         typename B>
D_INLINE_VAR D_CONSTEXPR_VAR bool are_same_system_v =
    are_same_system<A, B>::value;

template<typename A,
         typename B>
D_INLINE_VAR D_CONSTEXPR_VAR bool are_compatible_systems_v =
    are_compatible_systems<A, B>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


//==============================================================================
// 5.  POINT ALIASES
//==============================================================================
// The alias templates are C++11's; below it, cartesian<N, T>::point_type is
// the name.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename ValueType = double>
using point_2d = cartesian_point<2, ValueType>;

template<typename ValueType = double>
using point_3d = cartesian_point<3, ValueType>;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COORDINATE_COORDINATE_HPP
