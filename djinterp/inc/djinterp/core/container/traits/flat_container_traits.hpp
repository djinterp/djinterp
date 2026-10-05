/*******************************************************************************
* djinterp [core]                                      flat_container_traits.hpp
*
*   SFINAE structural traits for the FLAT side of the structure axis - whether
* a
* container holds only leaves (depth 1, the leaf summand alone, no node
* summand
* F[T]). Flatness is the COMPLEMENT of hierarchy over container-shaped types:
* a
* container is flat exactly when it is not hierarchical.
*
*   The heavy lifting - the container-shape guard, the depth recursion, the
* node_type tell, and the structure_category tag reading - lives in
* hierarchical_container_traits.hpp; this header is thin by design. Because
* the
* flat verdict is the negation of is_hierarchical_container, every nuance of
* that
* verdict carries over for free: a flat tag that suppresses the weak depth
* heuristic makes the type read as flat here, while a node_type (the strong
* signal) keeps it hierarchical and so NOT flat - one cannot flatten a
* declared
* node summand by tagging.
*
*   A flat tag is thus the opt-in WAY to assert leaf-ness where only the depth
* heuristic would otherwise nest a container-shaped element (a string); it is
* consulted through is_hierarchical_container, not re-read here.  The axis is
* orthogonal to the other intrinsic axes.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/flat_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_FLAT_CONTAINER_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_FLAT_CONTAINER_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"                    // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"           // D_TYPE_TRAIT_* detection macros
#include "../../meta/flat.hpp"                             // flat tag (structure_category opt-in)
#include "./hierarchical_container_traits.hpp"    // is_container_shape, is_hierarchical_container,
                                                 //   container_depth,
                                                 // structure_kind


NS_DJINTERP


// ===========================================================================
// I.   Flat classification
// ===========================================================================

// is_flat_container
//   trait: true iff the type is a container that does NOT nest -
// container-shaped and not hierarchical (depth 1, leaves only). Defined as the
// complement of is_hierarchical_container over container-shaped types, so it
// inherits that verdict's signal precedence (node_type strong, depth heuristic
// weak and flat- tag-suppressible).
template<typename Type>
struct is_flat_container
    : std::integral_constant<bool,
            is_container_shape<clean_t<Type>>::value
         && !is_hierarchical_container<clean_t<Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_flat_container)

// is_strictly_flat_container
//   trait: true iff the container is flat AND its value_type-chain depth is
// exactly 1 - i.e. flat by the depth measure itself, not only by the absence
// of the hierarchy signals. (For an ordinary flat container the two coincide;
// they can differ only for a type whose flatness rests on a tag rather than
// depth.)
template<typename Type>
struct is_strictly_flat_container
    : std::integral_constant<bool,
            is_flat_container<clean_t<Type>>::value
         && (container_depth<clean_t<Type>>::value == 1)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_strictly_flat_container)


// ===========================================================================
// II.  Aggregate snapshot
// ===========================================================================

// flat_container_class
//   trait: one-shot snapshot of the structure (flat side) axis for `Type`.
// Gathers every
// signal and verdict this header computes into a single instantiation, so
// a caller that needs several of them pays for detection once rather than per
// query.
template<typename Type>
struct flat_container_class
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr bool        is_container =
        is_container_shape<clean_type>::value;
    static constexpr std::size_t depth =
        container_depth<clean_type>::value;
    static constexpr bool        is_flat =
        is_flat_container<clean_type>::value;
    static constexpr bool        is_strictly_flat =
        is_strictly_flat_container<clean_type>::value;

    // shared summary (from hierarchical_container_traits.hpp)
    static constexpr structure_kind kind =
        structure_kind_of<clean_type>::value;
    static constexpr const char*    kind_name =
        structure_kind_name(kind);
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_FLAT_CONTAINER_TRAITS_HPP
