/*******************************************************************************
* djinterp [cpp]                                           interval_protocol.hpp
*
* The interval protocol, checked by its shape.
*   A container states a bound on its size, depth or multiplicity by naming a
* type as its size_interval, depth_interval or multiplicity_interval. Any type
* with math::interval's shape serves: static lower_bound and upper_bound
* members, and static bool is_left_open and is_right_open. This header checks
* that shape without including math, so that container, a stable module,
* does not depend on math, a preview one: math::interval satisfies the
* protocol because it has the shape, not because container names it.
*   closed_bounds is the smallest type with the shape, for a container that
* states a closed range of sizes without math.
*   C++11 and up; below C++11 this file is empty, as the container traits it
* serves are.
*
*
* path:      /inc/djinterp/cpp/container/traits/interval_protocol.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PROTOCOL
    ------------
    1.  Detection
         1.  is_interval_protocol
    2.  The smallest interval
         1.  closed_bounds
*/

#ifndef DJINTERP_CONTAINER_TRAITS_INTERVAL_PROTOCOL_HPP
#define DJINTERP_CONTAINER_TRAITS_INTERVAL_PROTOCOL_HPP 1

// djinterp
#include "../../djinterp.hpp"            // framework root
#include "../../meta/trait_detect.hpp"   // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>  // std::integral_constant, std::is_pointer, ...


NS_DJINTERP


//==============================================================================
// 1.  THE PROTOCOL
//==============================================================================
// math::interval's shape, as math's is_interval checks it, so that a type
// satisfies one exactly when it satisfies the other.


// 1.1    Detection
//------------------------------------------------------------------------------
NS_INTERNAL

    // interval_static_member
    //   trait: whether a member's address, as `&Type::name` gives it, is a
    // plain pointer: a static member's address is one, a non-static member's
    // is a pointer to member.
    template<typename Pointer>
    struct interval_static_member
        : std::integral_constant<bool,
              std::is_pointer<Pointer>::value>
    {};

    // interval_static_bool
    //   trait: whether a member's address is that of a static bool.
    template<typename Pointer>
    struct interval_static_bool
        : std::integral_constant<bool,
              ( (std::is_pointer<Pointer>::value) &&
                (std::is_same<typename std::remove_cv<
                                  typename std::remove_pointer<
                                      Pointer>::type>::type,
                              bool>::value) )>
    {};

    // interval_protocol_check
    //   trait: false where one of the four members cannot be named; where all
    // can, whether each is static, and the two openness flags bool.
    template<typename Type,
             typename = void>
    struct interval_protocol_check : std::false_type
    {};

    template<typename Type>
    struct interval_protocol_check<Type,
        D_VOID_T<decltype(&Type::lower_bound),
                 decltype(&Type::upper_bound),
                 decltype(&Type::is_left_open),
                 decltype(&Type::is_right_open)>>
        : std::integral_constant<bool,
              ( (interval_static_member<
                     decltype(&Type::lower_bound)>::value)  &&
                (interval_static_member<
                     decltype(&Type::upper_bound)>::value)  &&
                (interval_static_bool<
                     decltype(&Type::is_left_open)>::value) &&
                (interval_static_bool<
                     decltype(&Type::is_right_open)>::value) )>
    {};

NS_END  // internal

// 1.1.1
// is_interval_protocol
//   trait: whether Type has the interval protocol's shape: static
// lower_bound and upper_bound members, and static bool is_left_open and
// is_right_open. cv and references are removed first.
template<typename Type>
struct is_interval_protocol
    : internal::interval_protocol_check<
          typename std::remove_cv<
              typename std::remove_reference<Type>::type>::type>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_interval_protocol)


// 1.2    The smallest interval
//------------------------------------------------------------------------------
// 1.2.1
// closed_bounds
//   trait: the closed range [Lower, Upper] of a Value, as the interval
// protocol states it: what a container names as its size_interval,
// depth_interval or multiplicity_interval where it needs no more of an
// interval than its two bounds.
template<typename Value,
         Value    Lower,
         Value    Upper>
struct closed_bounds
{
    typedef Value value_type;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type upper_bound   = Upper;
    static D_CONSTEXPR_VAR bool       is_left_open  = false;
    static D_CONSTEXPR_VAR bool       is_right_open = false;

    D_STATIC_ASSERT((Lower <= Upper),
                    "closed_bounds: the lower bound exceeds the upper bound");
};

// the static members' definitions, which an odr-use needs below C++17; from
// C++17 a constexpr static member is inline, and a definition is redundant
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename Value,
         Value    Lower,
         Value    Upper>
D_CONSTEXPR_VAR Value closed_bounds<Value, Lower, Upper>::lower_bound;

template<typename Value,
         Value    Lower,
         Value    Upper>
D_CONSTEXPR_VAR Value closed_bounds<Value, Lower, Upper>::upper_bound;

template<typename Value,
         Value    Lower,
         Value    Upper>
D_CONSTEXPR_VAR bool closed_bounds<Value, Lower, Upper>::is_left_open;

template<typename Value,
         Value    Lower,
         Value    Upper>
D_CONSTEXPR_VAR bool closed_bounds<Value, Lower, Upper>::is_right_open;
#endif  // below C++17


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_CONTAINER_TRAITS_INTERVAL_PROTOCOL_HPP
