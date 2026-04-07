/******************************************************************************
* djinterp [container]                                       map_concepts.hpp
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
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Map Identity Concepts
* 3.   Lookup Concepts
* 4.   Mutation Concepts
* 5.   Supplementary Detection Concepts
*
* path:      /inc/container/meta/map_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                 date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_MAP_CONCEPTS_
#define DJINTERP_MAP_CONCEPTS_ 1

#include <type_traits>
#include "map_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "map_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Core Map Identity Concepts
// =============================================================================

// map_container
//   concept: constrains map-like types exposing key_type and mapped_type.
template<typename _Type>
concept map_container =
    is_map_structured_v<clean_t<_Type>>;

// non_map_container
//   concept: constrains types that are not classified as map-like.
template<typename _Type>
concept non_map_container =
    !map_container<_Type>;

// pair_valued_map
//   concept: constrains map-like types whose value_type is
// std::pair<const key_type, mapped_type>.
template<typename _Type>
concept pair_valued_map =
    has_map_pair_element_v<clean_t<_Type>>;

// unique_key_map
//   concept: constrains map-like types enforcing key uniqueness.
template<typename _Type>
concept unique_key_map =
    is_unique_key_map_v<clean_t<_Type>>;

// sorted_map
//   concept: constrains map-like types maintaining sorted key ordering.
template<typename _Type>
concept sorted_map =
    is_sorted_map_v<clean_t<_Type>>;

// hashed_map
//   concept: constrains map-like types using hash-based lookup.
template<typename _Type>
concept hashed_map =
    is_hashed_map_v<clean_t<_Type>>;

// overlay_map
//   concept: constrains types recognized as map overlays.
template<typename _Type>
concept overlay_map =
    is_map_overlay_v<clean_t<_Type>>;

// enum_key_map
//   concept: constrains map-like types whose key_type is an enum.
template<typename _Type>
concept enum_key_map =
    has_enum_key_v<clean_t<_Type>>;

// scoped_enum_key_map
//   concept: constrains map-like types whose key_type is a scoped enum.
template<typename _Type>
concept scoped_enum_key_map =
    has_scoped_enum_key_v<clean_t<_Type>>;

// homogeneous_value_map
//   concept: constrains map-like types whose mapped values are homogeneous.
template<typename _Type>
concept homogeneous_value_map =
    has_homogeneous_values_v<clean_t<_Type>>;


// =============================================================================
// II.  Lookup Concepts
// =============================================================================

// findable_map
//   concept: constrains map-like types exposing find(key).
template<typename _Type>
concept findable_map =
    has_map_find_v<clean_t<_Type>>;

// countable_map
//   concept: constrains map-like types exposing count(key).
template<typename _Type>
concept countable_map =
    has_map_count_v<clean_t<_Type>>;

// contains_map
//   concept: constrains map-like types exposing contains(key).
template<typename _Type>
concept contains_map =
    has_map_contains_v<clean_t<_Type>>;

// at_map
//   concept: constrains map-like types exposing at(key).
template<typename _Type>
concept at_map =
    has_map_at_v<clean_t<_Type>>;

// subscriptable_map
//   concept: constrains map-like types exposing operator[](key).
template<typename _Type>
concept subscriptable_map =
    has_map_subscript_v<clean_t<_Type>>;

// lower_bound_map
//   concept: constrains map-like types exposing lower_bound(key).
template<typename _Type>
concept lower_bound_map =
    has_map_lower_bound_v<clean_t<_Type>>;

// upper_bound_map
//   concept: constrains map-like types exposing upper_bound(key).
template<typename _Type>
concept upper_bound_map =
    has_map_upper_bound_v<clean_t<_Type>>;

// equal_range_map
//   concept: constrains map-like types exposing equal_range(key).
template<typename _Type>
concept equal_range_map =
    has_map_equal_range_v<clean_t<_Type>>;

// full_lookup_map
//   concept: constrains map-like types providing the full basic lookup set.
template<typename _Type>
concept full_lookup_map =
    map_does_full_lookup<clean_t<_Type>>;

// ordered_lookup_map
//   concept: constrains map-like types providing ordered-range lookup.
template<typename _Type>
concept ordered_lookup_map =
    map_does_ordered_lookup<clean_t<_Type>>;


// =============================================================================
// III. Mutation Concepts
// =============================================================================

// insertable_map
//   concept: constrains map-like types exposing insert(value_type).
template<typename _Type>
concept insertable_map =
    has_map_insert_v<clean_t<_Type>>;

// insert_or_assign_map
//   concept: constrains map-like types exposing insert_or_assign(key, value).
template<typename _Type>
concept insert_or_assign_map =
    has_map_insert_or_assign_v<clean_t<_Type>>;

// try_emplacing_map
//   concept: constrains map-like types exposing try_emplace(key).
template<typename _Type>
concept try_emplacing_map =
    has_map_try_emplace_v<clean_t<_Type>>;

// erase_key_map
//   concept: constrains map-like types exposing erase(key).
template<typename _Type>
concept erase_key_map =
    has_map_erase_key_v<clean_t<_Type>>;

// full_mutation_map
//   concept: constrains map-like types providing the core mutation set.
template<typename _Type>
concept full_mutation_map =
    map_does_full_mutation<clean_t<_Type>>;


// =============================================================================
// IV.  Supplementary Detection Concepts
// =============================================================================

// key_compare_map
//   concept: constrains map-like types exposing key_comp().
template<typename _Type>
concept key_compare_map =
    is_detected_v<map_key_comp_expr_t, clean_t<_Type>>;

// value_compare_map
//   concept: constrains map-like types exposing value_comp().
template<typename _Type>
concept value_compare_map =
    is_detected_v<map_value_comp_expr_t, clean_t<_Type>>;

// overlay_strategy_map
//   concept: constrains types exposing overlay_strategy.
template<typename _Type>
concept overlay_strategy_map =
    is_detected_v<map_overlay_strategy_expr_t, clean_t<_Type>>;

// backing_typed_map
//   concept: constrains types exposing backing_container_type.
template<typename _Type>
concept backing_typed_map =
    is_detected_v<map_backing_type_expr_t, clean_t<_Type>>;

// classified_map
//   concept: constrains types recognized by at least one public map trait.
template<typename _Type>
concept classified_map =
    ( map_container<_Type>             ||
      pair_valued_map<_Type>           ||
      overlay_map<_Type>               ||
      sorted_map<_Type>                ||
      hashed_map<_Type>                ||
      findable_map<_Type>              ||
      insertable_map<_Type> );


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_MAP_CONCEPTS_
