/*******************************************************************************
* djinterp [math]                                            interval_common.hpp
*
* What the four interval templates share: the road from a value type to the
* kernels that compute with it, and the iterator over their members.
*   The interval templates are faces over the C core, c/math/interval.h. Each
* operation of each template forwards to internal::interval_kernel<Type>,
* which is one of three things:
*
*     - for a signed integer type no wider than d_math_imax, the C core's
*       _imax family, reached by a conversion that loses nothing;
*     - for an unsigned integer type, bool among them, the _umax family;
*     - for float, double and long double (C++20 bounds), the _f, _d and _ld
*       families of c/math/interval_float.h, through the overloads that name
*       each kernel once for all three: an interval's members are the values
*       its type represents within its bounds, as for the integers;
*     - for anything else -- an enumeration, a structural class type
*       (C++20), an extended integer wider than the C families -- the generic
*       path below: the same operations written in the value type's own
*       arithmetic, as the templates computed them before the C core existed.
*       It is the one second implementation in the module, kept by the
*       owner's ruling because C has no generics; the face test holds it to
*       the C families wherever both can serve a type.
*
*   ITERATION is by index: the member under an iterator is the interval's
* at(index), and end() is index size(). Stepping a value instead, as the
* templates once did, cannot represent one past the type's maximum -- a
* closed_interval<uint8_t, 0, 255> iterated nothing -- and ++ on a bool is
* ill-formed from C++17.
*
*   EVERY LEVEL. Nothing here needs more than C++98. The kernels are
* constexpr from C++14 (D_MATH_FN), so the templates' operations are too.
*
*
* path:      /inc/djinterp/math/interval/interval_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FAMILY DISPATCH
    ---------------
    1.  Families
         1.  interval_family_of
         2.  interval_bounds
         3.  interval_bound_step
    2.  Kernel adapters
         1.  interval_kernel (signed C family)
         2.  interval_kernel (unsigned C family)
         3.  interval_kernel (floating-point C families)
2.  THE GENERIC PATH
    ----------------
    1.  Layout
         1.  generic_interval
    2.  Helpers
         1.  generic_interval_stride
         2.  generic_interval_text
    3.  Kernel
         1.  interval_kernel (generic)
3.  ITERATION
    ---------
    1.  Iterator
         1.  interval_iterator
*/

#ifndef DJINTERP_MATH_INTERVAL_INTERVAL_COMMON_HPP
#define DJINTERP_MATH_INTERVAL_INTERVAL_COMMON_HPP 1

// std
#include <cmath>     // std::fmod
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <cstdio>    // std::sprintf
#include <iterator>  // std::forward_iterator_tag
#include <string>    // std::string
// djinterp
#include "../../djinterp.hpp"         // framework root
#include "../../c/math/interval_float.h"  // the C core: d_interval_imax,
                                          // _umax, _f, _d, _ld
// re_std
#include "../../../re_std/type_traits/is_floating_point.hpp"  // the generic
                                                              // path's text
#include "../../../re_std/type_traits/is_integral.hpp"        // is_integral
#include "../../../re_std/type_traits/is_same.hpp"            // is_same
#include "../../../re_std/type_traits/is_signed.hpp"          // is_signed


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  FAMILY DISPATCH
//==============================================================================


// 1.1    Families
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // interval_family_of
    //   trait: which kernels serve an interval over Type: 1 for the C core's
    // signed family, 2 for its unsigned family, 3, 4 and 5 for its float,
    // double and long double families, 0 for the generic path. The sign is
    // asked only of an integer type: is_signed of an enumeration is not a
    // constant expression to every C++98 compiler.
    template<typename Type,
             bool     IsIntegral = re_std::is_integral<Type>::value>
    struct interval_family_of
    {
        enum
        {
            value = ( (re_std::is_same<Type, float>::value)       ? 3 :
                      (re_std::is_same<Type, double>::value)      ? 4 :
                      (re_std::is_same<Type, long double>::value) ? 5 :
                                                                    0 )
        };
    };

    template<typename Type>
    struct interval_family_of<Type, true>
    {
        enum
        {
            value = ( (sizeof(Type) > sizeof(d_math_umax)) ? 0 :
                      (re_std::is_signed<Type>::value)     ? 1 :
                                                             2 )
        };
    };

    // 1.1.2
    // interval_bounds
    //   function: the C core's bound flags for a pair of openness flags.
    D_CONSTEXPR inline unsigned
    interval_bounds(
        bool _left_open,
        bool _right_open
    ) D_NOEXCEPT
    {
        return ( (_left_open  ? static_cast<unsigned>(D_INTERVAL_LEFT_OPEN)
                              : 0u) |
                 (_right_open ? static_cast<unsigned>(D_INTERVAL_RIGHT_OPEN)
                              : 0u) );
    }

    // 1.1.3
    // interval_bound_step
    //   trait: a bound's neighbours, one member up (::up) and down (::down),
    // as the unified interval's inclusive bounds and conversion types need
    // them. An integer bound steps in the unsigned family and converts back,
    // so a bound at its type's end wraps, as an unsigned one always has,
    // instead of overflowing a constant expression: interval<int, 0, INT_MAX>
    // is a type at all. A floating-point bound (C++20) moves to the
    // neighbouring value, through float_order.h's constexpr next_up and
    // next_down; anything else by 1.
    template<typename Type,
             Type     Value,
             int      Kind = ( re_std::is_integral<Type>::value      ? 1 :
                               (interval_family_of<Type>::value >= 3) ? 2 :
                                                                       0 )>
    struct interval_bound_step
    {
        static D_CONSTEXPR_VAR Type up   = static_cast<Type>(Value + 1);
        static D_CONSTEXPR_VAR Type down = static_cast<Type>(Value - 1);
    };

    template<typename Type,
             Type     Value>
    struct interval_bound_step<Type, Value, 1>
    {
        static D_CONSTEXPR_VAR Type up   =
            static_cast<Type>(static_cast<d_math_umax>(Value) + 1u);
        static D_CONSTEXPR_VAR Type down =
            static_cast<Type>(static_cast<d_math_umax>(Value) - 1u);
    };

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // (a floating-point template argument exists only from C++20, and C++98
    // compilers reject a call in the initializer even of a template's member)
    template<typename Type,
             Type     Value>
    struct interval_bound_step<Type, Value, 2>
    {
        static constexpr Type up   = d_math_next_up(Value);
        static constexpr Type down = d_math_next_down(Value);
    };
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename Type, Type Value, int Kind>
    D_CONSTEXPR_VAR Type interval_bound_step<Type, Value, Kind>::up;
    template<typename Type, Type Value, int Kind>
    D_CONSTEXPR_VAR Type interval_bound_step<Type, Value, Kind>::down;
    template<typename Type, Type Value>
    D_CONSTEXPR_VAR Type interval_bound_step<Type, Value, 1>::up;
    template<typename Type, Type Value>
    D_CONSTEXPR_VAR Type interval_bound_step<Type, Value, 1>::down;
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    template<typename Type, Type Value>
    constexpr Type interval_bound_step<Type, Value, 2>::up;
    template<typename Type, Type Value>
    constexpr Type interval_bound_step<Type, Value, 2>::down;
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    // interval_kernel
    //   trait: the operations of an interval over Type, chosen by its family.
    template<typename Type,
             int      Family = interval_family_of<Type>::value>
    struct interval_kernel;

NS_END  // internal

// 1.2    Kernel adapters
//------------------------------------------------------------------------------
// Each adapter forwards, one line per operation, and converts the family's
// results back to Type; nothing is computed here.
NS_INTERNAL

    // 1.2.1
    // interval_kernel<Type, 1>
    //   struct: a signed integer type's operations: the _imax family.
    template<typename Type>
    struct interval_kernel<Type, 1>
    {
        typedef struct d_interval_imax core_type;

        static D_CONSTEXPR_CPP14 core_type
        make(
            Type     _lower,
            Type     _upper,
            unsigned _bounds,
            Type     _step
        ) D_NOEXCEPT
        {
            return d_interval_imax_make(_lower, _upper, _bounds, _step);
        }

        static D_CONSTEXPR_CPP14 bool
        is_valid(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_imax_is_valid(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_discrete(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_imax_is_discrete(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_empty(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_imax_is_empty(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_lower(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_inclusive_lower(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_upper(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_inclusive_upper(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        first(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_first(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        last(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_last(_core));
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        count(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_imax_count(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        contains(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_contains(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        contains_in_range(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_contains_in_range(_core, _value);
        }

        static D_CONSTEXPR_CPP14 Type
        clamp(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_clamp(_core, _value));
        }

        static D_CONSTEXPR_CPP14 Type
        clamp_nearest(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return static_cast<Type>(
                d_interval_imax_clamp_nearest(_core, _value));
        }

        static D_CONSTEXPR_CPP14 Type
        at(
            const core_type& _core,
            d_math_umax      _index
        ) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_imax_at(_core, _index));
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        index_of(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_index_of(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_imax_overlaps(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps_continuous(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_imax_overlaps_continuous(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        intersects(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_imax_intersects(_left, _right);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_normalize_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_effective_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_normalize_effective_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_discrete_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_imax_normalize_discrete_terms(_core, _value);
        }

        static std::string
        to_string(const core_type& _core)
        {
            char text[96];

            (void)d_interval_imax_format(text, sizeof(text), _core);

            return std::string(text);
        }
    };

    // 1.2.2
    // interval_kernel<Type, 2>
    //   struct: an unsigned integer type's operations: the _umax family.
    template<typename Type>
    struct interval_kernel<Type, 2>
    {
        typedef struct d_interval_umax core_type;

        static D_CONSTEXPR_CPP14 core_type
        make(
            Type     _lower,
            Type     _upper,
            unsigned _bounds,
            Type     _step
        ) D_NOEXCEPT
        {
            return d_interval_umax_make(_lower, _upper, _bounds, _step);
        }

        static D_CONSTEXPR_CPP14 bool
        is_valid(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_umax_is_valid(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_discrete(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_umax_is_discrete(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_empty(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_umax_is_empty(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_lower(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_inclusive_lower(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_upper(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_inclusive_upper(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        first(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_first(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        last(const core_type& _core) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_last(_core));
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        count(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_umax_count(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        contains(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_contains(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        contains_in_range(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_contains_in_range(_core, _value);
        }

        static D_CONSTEXPR_CPP14 Type
        clamp(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_clamp(_core, _value));
        }

        static D_CONSTEXPR_CPP14 Type
        clamp_nearest(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return static_cast<Type>(
                d_interval_umax_clamp_nearest(_core, _value));
        }

        static D_CONSTEXPR_CPP14 Type
        at(
            const core_type& _core,
            d_math_umax      _index
        ) D_NOEXCEPT
        {
            return static_cast<Type>(d_interval_umax_at(_core, _index));
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        index_of(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_index_of(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_umax_overlaps(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps_continuous(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_umax_overlaps_continuous(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        intersects(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_umax_intersects(_left, _right);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_normalize_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_effective_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_normalize_effective_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_discrete_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_umax_normalize_discrete_terms(_core, _value);
        }

        static std::string
        to_string(const core_type& _core)
        {
            char text[96];

            (void)d_interval_umax_format(text, sizeof(text), _core);

            return std::string(text);
        }
    };


    // 1.2.3
    // interval_kernel<float, 3>, <double, 4>, <long double, 5>
    //   struct: a floating-point type's operations: the _f, _d and _ld
    // families, reached through the C++ overloads interval_float.h gives
    // each kernel, so one template serves the three.
    template<typename Type>
    struct interval_float_core;

    template<>
    struct interval_float_core<float>
    {
        typedef struct d_interval_f type;
    };

    template<>
    struct interval_float_core<double>
    {
        typedef struct d_interval_d type;
    };

    template<>
    struct interval_float_core<long double>
    {
        typedef struct d_interval_ld type;
    };

    template<typename Type>
    struct interval_float_kernel
    {
        typedef typename interval_float_core<Type>::type core_type;

        static D_CONSTEXPR_CPP14 core_type
        make(
            Type     _lower,
            Type     _upper,
            unsigned _bounds,
            Type     _step
        ) D_NOEXCEPT
        {
            return d_interval_make(_lower, _upper, _bounds, _step);
        }

        static D_CONSTEXPR_CPP14 bool
        is_valid(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_is_valid(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_discrete(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_is_discrete(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        is_empty(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_is_empty(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_lower(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_inclusive_lower(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_upper(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_inclusive_upper(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        first(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_first(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        last(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_last(_core);
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        count(const core_type& _core) D_NOEXCEPT
        {
            return d_interval_count(_core);
        }

        static D_CONSTEXPR_CPP14 bool
        contains(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_contains(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        contains_in_range(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_contains_in_range(_core, _value);
        }

        static D_CONSTEXPR_CPP14 Type
        clamp(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_clamp(_core, _value);
        }

        static D_CONSTEXPR_CPP14 Type
        clamp_nearest(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_clamp_nearest(_core, _value);
        }

        static D_CONSTEXPR_CPP14 Type
        at(
            const core_type& _core,
            d_math_umax      _index
        ) D_NOEXCEPT
        {
            return d_interval_at(_core, _index);
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        index_of(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_index_of(_core, _value);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_overlaps(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps_continuous(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_overlaps_continuous(_left, _right);
        }

        static D_CONSTEXPR_CPP14 bool
        intersects(
            const core_type& _left,
            const core_type& _right
        ) D_NOEXCEPT
        {
            return d_interval_intersects(_left, _right);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_normalize_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_effective_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_normalize_effective_terms(_core, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_discrete_terms(
            const core_type& _core,
            Type             _value
        ) D_NOEXCEPT
        {
            return d_interval_normalize_discrete_terms(_core, _value);
        }

        static std::string
        to_string(const core_type& _core)
        {
            char text[128];

            (void)d_interval_format(text, sizeof(text), _core);

            return std::string(text);
        }
    };

    template<>
    struct interval_kernel<float, 3> : interval_float_kernel<float>
    {};

    template<>
    struct interval_kernel<double, 4> : interval_float_kernel<double>
    {};

    template<>
    struct interval_kernel<long double, 5>
        : interval_float_kernel<long double>
    {};

NS_END  // internal


//==============================================================================
// 2.  THE GENERIC PATH
//==============================================================================
// The operations in Type's own arithmetic, for a type no C family holds. The
// expressions are the templates' former ones, so an enumeration keeps its
// meaning; where they now differ is where the C core is right and they were
// not: emptiness in overlaps, and a stride on a floating-point type no C
// family holds, which `%` cannot take. intersects tests each member of the
// smaller interval in the other. float, double and long double take the C
// families (1.2.3).


// 2.1    Layout
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // generic_interval
    //   struct: an interval over Type, laid out as the C core's are.
    template<typename Type>
    struct generic_interval
    {
        Type     lower;
        Type     upper;
        Type     step;
        unsigned bounds;
    };

NS_END  // internal

// 2.2    Helpers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // generic_interval_stride
    //   struct: whether an offset is a whole number of steps: `%` for an
    // integer or enumeration, std::fmod for a floating-point type.
    template<bool IsFloating>
    struct generic_interval_stride
    {
        template<typename Offset,
                 typename Step>
        static D_CONSTEXPR_CPP14 bool
        aligned(
            Offset _offset,
            Step   _step
        )
        {
            return ((_offset % _step) == 0);
        }
    };

    template<>
    struct generic_interval_stride<true>
    {
        template<typename Offset,
                 typename Step>
        static bool
        aligned(
            Offset _offset,
            Step   _step
        )
        {
            return (std::fmod(_offset, _step) == 0);
        }
    };

    // 2.2.2
    // generic_interval_text
    //   struct: one bound as text, as std::to_string writes it: "%f" for a
    // floating-point type, decimal for an integer or enumeration.
    template<typename Type,
             bool     IsFloating = re_std::is_floating_point<Type>::value>
    struct generic_interval_text
    {
        static std::string
        of(Type _value)
        {
            char              text[64];
            const std::size_t length =
                d_internal_interval_put_imax(text,
                                             sizeof(text),
                                             0u,
                                             static_cast<d_math_imax>(_value));

            (void)d_internal_interval_finish(text, sizeof(text), length);

            return std::string(text);
        }
    };

    template<typename Type>
    struct generic_interval_text<Type, true>
    {
        static std::string
        of(Type _value)
        {
            // "%f" of the largest long double runs to some 4,950 digits
            char text[5120];

            // long double has its own conversion; float and double share one
            if (re_std::is_same<Type, long double>::value)
            {
                (void)std::sprintf(text,
                                   "%Lf",
                                   static_cast<long double>(_value));
            }
            else
            {
                (void)std::sprintf(text, "%f", static_cast<double>(_value));
            }

            return std::string(text);
        }
    };

NS_END  // internal

// 2.3    Kernel
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.3.1
    // interval_kernel<Type, 0>
    //   struct: the generic path's operations, in Type's own arithmetic.
    template<typename Type>
    struct interval_kernel<Type, 0>
    {
        typedef generic_interval<Type>                            core_type;
        typedef generic_interval_stride<
                    re_std::is_floating_point<Type>::value>       stride;

        static D_CONSTEXPR_CPP14 core_type
        make(
            Type     _lower,
            Type     _upper,
            unsigned _bounds,
            Type     _step
        )
        {
            core_type result = { _lower, _upper, _step, _bounds };

            return result;
        }

        static D_CONSTEXPR_CPP14 bool
        is_valid(const core_type& _core)
        {
            return ( (_core.lower <= _core.upper) &&
                     (!(_core.step < static_cast<Type>(0))) );
        }

        static D_CONSTEXPR_CPP14 bool
        is_discrete(const core_type& _core)
        {
            return (_core.step > static_cast<Type>(0));
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_lower(const core_type& _core)
        {
            return ((_core.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                       ? static_cast<Type>(_core.lower + 1)
                       : _core.lower;
        }

        static D_CONSTEXPR_CPP14 Type
        inclusive_upper(const core_type& _core)
        {
            return ((_core.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                       ? static_cast<Type>(_core.upper - 1)
                       : _core.upper;
        }

        static D_CONSTEXPR_CPP14 bool
        is_empty(const core_type& _core)
        {
            return (inclusive_lower(_core) > inclusive_upper(_core));
        }

        static D_CONSTEXPR_CPP14 Type
        first(const core_type& _core)
        {
            return inclusive_lower(_core);
        }

        static D_CONSTEXPR_CPP14 Type
        last(const core_type& _core)
        {
            const Type low  = inclusive_lower(_core);
            const Type high = inclusive_upper(_core);

            // a discrete interval ends on its last whole step
            if (is_discrete(_core))
            {
                return static_cast<Type>(
                    low + ((high - low) / _core.step) * _core.step);
            }

            return high;
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        count(const core_type& _core)
        {
            const Type low  = inclusive_lower(_core);
            const Type high = inclusive_upper(_core);

            // an empty interval counts nothing
            if (low > high)
            {
                return 0u;
            }

            // a discrete interval counts its whole steps
            if (is_discrete(_core))
            {
                return static_cast<d_math_umax>(
                    (high - low) / _core.step + 1);
            }

            return static_cast<d_math_umax>(high - low + 1);
        }

        static D_CONSTEXPR_CPP14 bool
        contains_in_range(
            const core_type& _core,
            Type             _value
        )
        {
            const bool left_ok =
                ((_core.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                    ? (_value > _core.lower)
                    : (_value >= _core.lower);
            const bool right_ok =
                ((_core.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                    ? (_value < _core.upper)
                    : (_value <= _core.upper);

            return (left_ok && right_ok);
        }

        static D_CONSTEXPR_CPP14 bool
        contains(
            const core_type& _core,
            Type             _value
        )
        {
            // outside the bounds is outside the interval
            if (!contains_in_range(_core, _value))
            {
                return false;
            }

            // a discrete member is a whole number of steps past the first
            if (is_discrete(_core))
            {
                return stride::aligned(_value - inclusive_lower(_core),
                                       _core.step);
            }

            return true;
        }

        static D_CONSTEXPR_CPP14 Type
        clamp(
            const core_type& _core,
            Type             _value
        )
        {
            const Type low  = inclusive_lower(_core);
            const Type high = inclusive_upper(_core);

            // below the interval: its first value
            if (_value < low)
            {
                return low;
            }

            // above the interval: its last value
            if (_value > high)
            {
                return last(_core);
            }

            // inside a discrete interval: down to a whole step
            if (is_discrete(_core))
            {
                return static_cast<Type>(
                    low + ((_value - low) / _core.step) * _core.step);
            }

            return _value;
        }

        static D_CONSTEXPR_CPP14 Type
        clamp_nearest(
            const core_type& _core,
            Type             _value
        )
        {
            // a continuous interval has no steps to round between
            if (!is_discrete(_core))
            {
                return clamp(_core, _value);
            }

            const Type low = inclusive_lower(_core);

            // below the interval: its first value
            if (_value < low)
            {
                return low;
            }

            const Type top = last(_core);

            // above the last step: the last step
            if (_value > top)
            {
                return top;
            }

            const Type below = static_cast<Type>(
                low + ((_value - low) / _core.step) * _core.step);
            const Type above = static_cast<Type>(below + _core.step);

            // the step above is nearer, and still inside the interval
            if ( (above <= inclusive_upper(_core))        &&
                 ((_value - below) > (above - _value)) )
            {
                return above;
            }

            return below;
        }

        static D_CONSTEXPR_CPP14 Type
        at(
            const core_type& _core,
            d_math_umax      _index
        )
        {
            // a continuous interval steps by 1
            if (is_discrete(_core))
            {
                return static_cast<Type>(
                    inclusive_lower(_core) +
                    static_cast<Type>(_index) * _core.step);
            }

            return static_cast<Type>(inclusive_lower(_core) +
                                     static_cast<Type>(_index));
        }

        static D_CONSTEXPR_CPP14 d_math_umax
        index_of(
            const core_type& _core,
            Type             _value
        )
        {
            // a value that is not a member has the index one past the last
            if (!contains(_core, _value))
            {
                return count(_core);
            }

            // a discrete index counts whole steps
            if (is_discrete(_core))
            {
                return static_cast<d_math_umax>(
                    (_value - inclusive_lower(_core)) / _core.step);
            }

            return static_cast<d_math_umax>(_value - inclusive_lower(_core));
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps(
            const core_type& _left,
            const core_type& _right
        )
        {
            // an empty interval shares a position with nothing
            if ( (is_empty(_left)) ||
                 (is_empty(_right)) )
            {
                return false;
            }

            return !( (inclusive_upper(_left) < inclusive_lower(_right)) ||
                      (inclusive_lower(_left) > inclusive_upper(_right)) );
        }

        static D_CONSTEXPR_CPP14 bool
        overlaps_continuous(
            const core_type& _left,
            const core_type& _right
        )
        {
            // an interval empty on the real line meets nothing
            if ( (is_void(_left)) ||
                 (is_void(_right)) )
            {
                return false;
            }

            const bool left_meets_closed =
                ( ((_left.bounds  & D_INTERVAL_LEFT_OPEN)  == 0u) &&
                  ((_right.bounds & D_INTERVAL_RIGHT_OPEN) == 0u) );
            const bool right_meets_closed =
                ( ((_left.bounds  & D_INTERVAL_RIGHT_OPEN) == 0u) &&
                  ((_right.bounds & D_INTERVAL_LEFT_OPEN)  == 0u) );
            const bool left_ok =
                left_meets_closed ? (_left.lower <= _right.upper)
                                  : (_left.lower <  _right.upper);
            const bool right_ok =
                right_meets_closed ? (_right.lower <= _left.upper)
                                   : (_right.lower <  _left.upper);

            return (left_ok && right_ok);
        }

        static D_CONSTEXPR_CPP14 bool
        intersects(
            const core_type& _left,
            const core_type& _right
        )
        {
            // two continuous intervals share a member where they overlap
            if ( (!is_discrete(_left)) &&
                 (!is_discrete(_right)) )
            {
                return overlaps(_left, _right);
            }

            const bool        left_walks = (count(_left) <= count(_right));
            const core_type&  walk       = left_walks ? _left  : _right;
            const core_type&  other      = left_walks ? _right : _left;
            const d_math_umax total      = count(walk);

            // each member of the smaller, tested in the other
            for (d_math_umax i = 0u; i < total; ++i)
            {
                if (contains(other, at(walk, i)))
                {
                    return true;
                }
            }

            return false;
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_terms(
            const core_type& _core,
            Type             _value
        )
        {
            return terms(_core.lower, _core.upper, _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_effective_terms(
            const core_type& _core,
            Type             _value
        )
        {
            return terms(inclusive_lower(_core),
                         inclusive_upper(_core),
                         _value);
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        normalize_discrete_terms(
            const core_type& _core,
            Type             _value
        )
        {
            // a continuous interval normalizes over its inclusive bounds
            if (!is_discrete(_core))
            {
                return normalize_effective_terms(_core, _value);
            }

            struct d_math_ratio result = { 0.0L, 1.0L };
            const d_math_umax   total  = count(_core);

            // one member or none: nothing to spread over
            if (total <= 1u)
            {
                return result;
            }

            const Type index = static_cast<Type>(
                (_value - inclusive_lower(_core)) / _core.step);

            result.numerator   = static_cast<long double>(index);
            result.denominator = static_cast<long double>(total - 1u);

            return result;
        }

        static std::string
        to_string(const core_type& _core)
        {
            const bool  discrete = is_discrete(_core);
            std::string result   =
                ((_core.bounds & D_INTERVAL_LEFT_OPEN) != 0u) ? "(" : "[";

            result += generic_interval_text<Type>::of(_core.lower);
            result += discrete ? ":" : ", ";

            // a discrete interval shows its stride between its bounds
            if (discrete)
            {
                result += generic_interval_text<Type>::of(_core.step);
                result += ":";
            }

            result += generic_interval_text<Type>::of(_core.upper);
            result += ((_core.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                          ? ")" : "]";

            return result;
        }

    private:
        static D_CONSTEXPR_CPP14 bool
        is_void(const core_type& _core)
        {
            return ( (_core.lower > _core.upper) ||
                     ( (!(_core.lower < _core.upper))     &&
                       (_core.bounds != D_INTERVAL_CLOSED) ) );
        }

        static D_CONSTEXPR_CPP14 struct d_math_ratio
        terms(
            Type _origin,
            Type _end,
            Type _value
        )
        {
            struct d_math_ratio result = { 0.0L, 1.0L };

            // a degenerate span maps everything to 0
            if (!(_origin < _end) && !(_end < _origin))
            {
                return result;
            }

            result.numerator   = static_cast<long double>(_value - _origin);
            result.denominator = static_cast<long double>(_end - _origin);

            return result;
        }
    };

NS_END  // internal


//==============================================================================
// 3.  ITERATION
//==============================================================================


// 3.1    Iterator
//------------------------------------------------------------------------------
// 3.1.1
// interval_iterator
//   class: forward iterator over an interval's members, by index: the member
// under it is Interval::at(index), so stepping never passes the end of the
// value type. Dereferencing yields the member by value.
template<typename Interval>
class interval_iterator
{
public:
    typedef std::forward_iterator_tag            iterator_category;
    typedef std::ptrdiff_t                       difference_type;
    typedef typename Interval::value_type        value_type;
    typedef const value_type*                    pointer;
    typedef value_type                           reference;
    typedef typename Interval::size_type         size_type;

    D_CONSTEXPR
    interval_iterator() D_NOEXCEPT
        : m_index(0)
    {}

    D_CONSTEXPR explicit
    interval_iterator(
        size_type _index
    ) D_NOEXCEPT
        : m_index(_index)
    {}

    D_CONSTEXPR_CPP14 value_type
    operator*() const D_NOEXCEPT
    {
        return Interval::at(m_index);
    }

    D_CONSTEXPR_CPP14 interval_iterator&
    operator++() D_NOEXCEPT
    {
        ++m_index;

        return *this;
    }

    D_CONSTEXPR_CPP14 interval_iterator
    operator++(int) D_NOEXCEPT
    {
        const interval_iterator previous(*this);

        ++m_index;

        return previous;
    }

    D_CONSTEXPR bool
    operator==(const interval_iterator& _other) const D_NOEXCEPT
    {
        return (m_index == _other.m_index);
    }

    D_CONSTEXPR bool
    operator!=(const interval_iterator& _other) const D_NOEXCEPT
    {
        return (m_index != _other.m_index);
    }

    // index
    //   query: the iterator's position, 0 at begin().
    D_CONSTEXPR size_type
    index() const D_NOEXCEPT
    {
        return m_index;
    }

private:
    size_type m_index;
};


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_INTERVAL_COMMON_HPP
