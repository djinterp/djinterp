/*******************************************************************************
* djinterp [core]                                               map_concepts.hpp
*
*  djinterp map classification concepts
*   C++20 concepts layered on top of map_traits.hpp.  These concepts
* provide readable `requires` constraints for map-like containers,
* including structural map identity, ordering, overlay detection,
* lookup capability, and mutation capability.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the map trait layer.
*
*
* path:      /inc/djinterp/core/container/map/map_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                  created: 2026.04.06
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    Feature Gate
      ------------

2.    Core Map Identity Concepts
      --------------------------

3.    Lookup Concepts
      ---------------

4.    Mutation Concepts
      -----------------

5.    Supplementary Detection Concepts
      --------------------------------
*/

#ifndef DJINTERP_CONTAINER_MAP_MAP_CONCEPTS_HPP
#define DJINTERP_CONTAINER_MAP_MAP_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "map_traits.hpp"

// below C++20 concepts this header is empty: a facility above its tier
// is absent, never an #error
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Core Map Identity Concepts
// =============================================================================

// map_container
//   concept: constrains map-like types exposing key_type and mapped_type.
template<typename Type>
concept map_container =
    is_map_structured_v<clean_t<Type>>;

// non_map_container
//   concept: constrains types that are not classified as map-like.
template<typename Type>
concept non_map_container =
    !map_container<Type>;

// pair_valued_map
//   concept: constrains map-like types whose value_type is std::pair<const
// key_type, mapped_type>.
template<typename Type>
concept pair_valued_map =
    has_map_pair_element_v<clean_t<Type>>;

// unique_key_map
//   concept: constrains map-like types enforcing key uniqueness.
template<typename Type>
concept unique_key_map =
    is_unique_key_map_v<clean_t<Type>>;

// sorted_map
//   concept: constrains map-like types maintaining sorted key ordering.
template<typename Type>
concept sorted_map =
    is_sorted_map_v<clean_t<Type>>;

// hashed_map
//   concept: constrains map-like types using hash-based lookup.
template<typename Type>
concept hashed_map =
    is_hashed_map_v<clean_t<Type>>;

// overlay_map
//   concept: constrains types recognized as map overlays.
template<typename Type>
concept overlay_map =
    is_map_overlay_v<clean_t<Type>>;

// enum_key_map
//   concept: constrains map-like types whose key_type is an enum.
template<typename Type>
concept enum_key_map =
    has_enum_key_v<clean_t<Type>>;

// scoped_enum_key_map
//   concept: constrains map-like types whose key_type is a scoped enum.
template<typename Type>
concept scoped_enum_key_map =
    has_scoped_enum_key_v<clean_t<Type>>;

// homogeneous_value_map
//   concept: constrains map-like types whose mapped values are homogeneous.
template<typename Type>
concept homogeneous_value_map =
    has_homogeneous_values_v<clean_t<Type>>;


// =============================================================================
// II.  Lookup Concepts
// =============================================================================

// findable_map
//   concept: constrains map-like types exposing find(key).
template<typename Type>
concept findable_map =
    has_map_find_v<clean_t<Type>>;

// countable_map
//   concept: constrains map-like types exposing count(key).
template<typename Type>
concept countable_map =
    has_map_count_v<clean_t<Type>>;

// contains_map
//   concept: constrains map-like types exposing contains(key).
template<typename Type>
concept contains_map =
    has_map_contains_v<clean_t<Type>>;

// at_map
//   concept: constrains map-like types exposing at(key).
template<typename Type>
concept at_map =
    has_map_at_v<clean_t<Type>>;

// subscriptable_map
//   concept: constrains map-like types exposing operator[](key).
template<typename Type>
concept subscriptable_map =
    has_map_subscript_v<clean_t<Type>>;

// lower_bound_map
//   concept: constrains map-like types exposing lower_bound(key).
template<typename Type>
concept lower_bound_map =
    has_map_lower_bound_v<clean_t<Type>>;

// upper_bound_map
//   concept: constrains map-like types exposing upper_bound(key).
template<typename Type>
concept upper_bound_map =
    has_map_upper_bound_v<clean_t<Type>>;

// equal_range_map
//   concept: constrains map-like types exposing equal_range(key).
template<typename Type>
concept equal_range_map =
    has_map_equal_range_v<clean_t<Type>>;

// full_lookup_map
//   concept: constrains map-like types providing the full basic lookup set.
template<typename Type>
concept full_lookup_map =
    map_does_full_lookup<clean_t<Type>>;

// ordered_lookup_map
//   concept: constrains map-like types providing ordered-range lookup.
template<typename Type>
concept ordered_lookup_map =
    map_does_ordered_lookup<clean_t<Type>>;


// =============================================================================
// III. Mutation Concepts
// =============================================================================

// insertable_map
//   concept: constrains map-like types exposing insert(value_type).
template<typename Type>
concept insertable_map =
    has_map_insert_v<clean_t<Type>>;

// insert_or_assign_map
//   concept: constrains map-like types exposing insert_or_assign(key, value).
template<typename Type>
concept insert_or_assign_map =
    has_map_insert_or_assign_v<clean_t<Type>>;

// try_emplacing_map
//   concept: constrains map-like types exposing try_emplace(key).
template<typename Type>
concept try_emplacing_map =
    has_map_try_emplace_v<clean_t<Type>>;

// erase_key_map
//   concept: constrains map-like types exposing erase(key).
template<typename Type>
concept erase_key_map =
    has_map_erase_key_v<clean_t<Type>>;

// full_mutation_map
//   concept: constrains map-like types providing the core mutation set.
template<typename Type>
concept full_mutation_map =
    map_does_full_mutation<clean_t<Type>>;


// =============================================================================
// IV.  Supplementary Detection Concepts
// =============================================================================

// key_compare_map
//   concept: constrains map-like types exposing key_comp().
template<typename Type>
concept key_compare_map =
    is_detected_v<map_key_comp_expr_t, clean_t<Type>>;

// value_compare_map
//   concept: constrains map-like types exposing value_comp().
template<typename Type>
concept value_compare_map =
    is_detected_v<map_value_comp_expr_t, clean_t<Type>>;

// overlay_strategy_map
//   concept: constrains types exposing overlay_strategy.
template<typename Type>
concept overlay_strategy_map =
    is_detected_v<map_overlay_strategy_expr_t, clean_t<Type>>;

// backing_typed_map
//   concept: constrains types exposing backing_container_type.
template<typename Type>
concept backing_typed_map =
    is_detected_v<map_backing_type_expr_t, clean_t<Type>>;

// classified_map
//   concept: constrains types recognized by at least one public map trait.
template<typename Type>
concept classified_map =
    ( map_container<Type>             ||
      pair_valued_map<Type>           ||
      overlay_map<Type>               ||
      sorted_map<Type>                ||
      hashed_map<Type>                ||
      findable_map<Type>              ||
      insertable_map<Type> );


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_MAP_MAP_CONCEPTS_HPP
