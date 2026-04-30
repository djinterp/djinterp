/******************************************************************************
* djinterp [container]                  flat_hierarchical_container_traits.hpp
*
* SFINAE structural traits for the flat / hierarchical axis.
*   Definition (project-canonical):
*   A container C of element type T is hierarchical iff an element E of C can 
* be either of type T or itself a container of type C.
*   Equivalently: hierarchical containers have value_type chains
* that don't immediately bottom out at a non-container leaf.  The
* recursion depth is bounded statically by _MaxDepth (default 32)
* so that pathologically self-referential types cannot cause
* compile-time recursion to exceed compiler limits.
*   Detection signals:
*     1. is_container_shape - has nested value_type AND a size()
*        accessor; the "looks like a container" guard used to
*        recurse into nested levels.
*     2. container_depth - recursive SFINAE walk over value_type
*        chains.  Reports 0 for non-container leaves, 1 for flat
*        containers, and >=2 for hierarchical ones.
*   The flat / hierarchical axis is orthogonal to all other axes.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/
*                flat_hierarchical_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_FLAT_HIERARCHICAL_CONTAINER_TRAITS_
#define DJINTERP_FLAT_HIERARCHICAL_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../env/cpp/env_cpp_features.h"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE shape detection
// ===========================================================================

NS_INTERNAL

    // is_container_shape
    //   helper: true if _Type quacks like a container (has
    // value_type and a size() accessor).
    template<typename _Type,
             typename = void>
    struct is_container_shape : std::false_type
    {};

    template<typename _Type>
    struct is_container_shape<_Type, void_t<
        typename _Type::value_type,
        decltype(std::declval<const _Type&>().size())
    >> : std::true_type
    {};

NS_END  // internal


// ===========================================================================
// II.  Recursive depth computation
// ===========================================================================

NS_INTERNAL

    // depth_recurse
    //   helper: counts the depth of nested-container value_types
    // bounded by _Remaining.  Returns 0 at non-container leaves;
    // 1 + recurse(value_type) at container nodes.
    template<typename _Type,
             std::size_t _Remaining,
             typename = void>
    struct depth_recurse
        : std::integral_constant<std::size_t, 0>
    {};

    template<typename _Type,
             std::size_t _Remaining>
    struct depth_recurse<_Type, _Remaining,
        typename std::enable_if<
                (_Remaining > 0)
             && is_container_shape<_Type>::value
        >::type>
        : std::integral_constant<std::size_t,
              1 + depth_recurse<
                  typename _Type::value_type,
                  _Remaining - 1
              >::value>
    {};

NS_END  // internal


// container_depth
//   trait: compile-time recursion depth.  Flat containers report
// 1; vector<vector<int>> reports 2; etc.  Returns 0 for non-
// container types.  _MaxDepth caps the recursion (default 32).
template<typename _Type,
         std::size_t _MaxDepth = 32>
struct container_depth
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr std::size_t value =
        internal::depth_recurse<clean_type, _MaxDepth>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             std::size_t _MaxDepth = 32>
    constexpr std::size_t container_depth_v =
        container_depth<_Type, _MaxDepth>::value;
#endif


// max_depth_of
//   trait: alias of container_depth for parity with similar
// "_of" trait names.
template<typename _Type,
         std::size_t _MaxDepth = 32>
struct max_depth_of
    : container_depth<_Type, _MaxDepth>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             std::size_t _MaxDepth = 32>
    constexpr std::size_t max_depth_of_v =
        max_depth_of<_Type, _MaxDepth>::value;
#endif


// ===========================================================================
// III. Classification umbrellas
// ===========================================================================

// is_flat_container
//   trait: true if container_depth<_Type> == 1 - i.e. the
// container's value_type is a non-container leaf.
template<typename _Type>
struct is_flat_container
    : std::integral_constant<bool,
          (container_depth<_Type>::value == 1)>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_flat_container_v =
        is_flat_container<_Type>::value;
#endif


// is_hierarchical_container
//   trait: true if container_depth<_Type> >= 2 - i.e. the
// container's value_type is itself a container.
template<typename _Type>
struct is_hierarchical_container
    : std::integral_constant<bool,
          (container_depth<_Type>::value >= 2)>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_hierarchical_container_v =
        is_hierarchical_container<_Type>::value;
#endif


// is_depth_bounded_container
//   trait: true if the container is hierarchical AND its depth is
// <= _N.  Useful for constraining functions to specific recursion
// budgets.
template<typename _Type,
         std::size_t _N>
struct is_depth_bounded_container
    : std::integral_constant<bool,
            (container_depth<_Type>::value >= 2)
         && (container_depth<_Type>::value <= _N)>
{};


// ===========================================================================
// IV.  Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct flat_hierarchical_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr std::size_t depth =
        container_depth<clean_type>::value;
    static constexpr bool is_flat =
        is_flat_container<clean_type>::value;
    static constexpr bool is_hierarchical =
is_hierarchical_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_FLAT_HIERARCHICAL_CONTAINER_TRAITS_