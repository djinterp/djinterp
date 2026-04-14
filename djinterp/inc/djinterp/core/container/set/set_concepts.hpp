/******************************************************************************
* djinterp [container]                                        set_concepts.hpp
*
* djinterp set container concepts header:
*   C++20 concepts layered over set_traits.hpp. These concepts provide
* readable constraints for set-like containers without replacing the
* existing SFINAE trait surface.
*
*   The concepts mirror the public classification axes from set_traits.hpp:
*   - core set-like structure
*   - ordered / unordered classification
*   - uniqueness / multiplicity classification
*   - flat-set detection
*   - set-specific method availability
*   - lookup / insertion / erasure strategy classification
*
*
* path:      /inc/djinterp/container/set/set_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_SET_CONCEPTS_
#define DJINTERP_SET_CONCEPTS_ 1

#ifndef __cplusplus
    #error "set_concepts.hpp requires C++ compilation"
#endif

#include "../../djinterp.hpp"
#include "./set_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
///              I.   CORE STRUCTURAL CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// set_like_container
//   concept: the type satisfies the structural set-like protocol.
template<typename _Type>
concept set_like_container =
    is_set_like_v<_Type>;

// ordered_set_container
//   concept: the type is a sorted set-like container.
template<typename _Type>
concept ordered_set_container =
    is_ordered_set_v<_Type>;

// unordered_set_container
//   concept: the type is a hash-based set-like container.
template<typename _Type>
concept unordered_set_container =
    is_unordered_set_v<_Type>;

// unique_set_container
//   concept: the type enforces unique keys.
template<typename _Type>
concept unique_set_container =
    is_unique_set_v<_Type>;

// multi_set_container
//   concept: the type allows duplicate keys.
template<typename _Type>
concept multi_set_container =
    is_multi_set_v<_Type>;

// flat_set_container
//   concept: the type follows the flat-set backing-container pattern.
template<typename _Type>
concept flat_set_container =
    is_flat_set_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
///              II.  COMPOUND CLASSIFICATION CONCEPTS                      ///
///////////////////////////////////////////////////////////////////////////////

// ordered_unique_set_container
//   concept: sorted + unique-key set.
template<typename _Type>
concept ordered_unique_set_container =
    is_ordered_unique_set_v<_Type>;

// ordered_multi_set_container
//   concept: sorted + duplicate-key set.
template<typename _Type>
concept ordered_multi_set_container =
    is_ordered_multi_set_v<_Type>;

// unordered_unique_set_container
//   concept: hashed + unique-key set.
template<typename _Type>
concept unordered_unique_set_container =
    is_unordered_unique_set_v<_Type>;

// unordered_multi_set_container
//   concept: hashed + duplicate-key set.
template<typename _Type>
concept unordered_multi_set_container =
    is_unordered_multi_set_v<_Type>;

// flat_unique_set_container
//   concept: flat backed + sorted + unique-key set.
template<typename _Type>
concept flat_unique_set_container =
    is_flat_unique_set_v<_Type>;

// flat_multi_set_container
//   concept: flat backed + sorted + duplicate-key set.
template<typename _Type>
concept flat_multi_set_container =
    is_flat_multi_set_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
///              III. SET OPERATION CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// findable_set_container
//   concept: the type exposes find(key_type).
template<typename _Type>
concept findable_set_container =
    has_set_find_v<_Type>;

// countable_set_container
//   concept: the type exposes count(key_type).
template<typename _Type>
concept countable_set_container =
    has_set_count_v<_Type>;

// contains_set_container
//   concept: the type exposes contains(key_type).
template<typename _Type>
concept contains_set_container =
    has_set_contains_v<_Type>;

// equal_range_set_container
//   concept: the type exposes equal_range(key_type).
template<typename _Type>
concept equal_range_set_container =
    has_set_equal_range_v<_Type>;

// lower_bound_set_container
//   concept: the type exposes lower_bound(key_type).
template<typename _Type>
concept lower_bound_set_container =
    has_set_lower_bound_v<_Type>;

// upper_bound_set_container
//   concept: the type exposes upper_bound(key_type).
template<typename _Type>
concept upper_bound_set_container =
    has_set_upper_bound_v<_Type>;

// range_lookup_set_container
//   concept: the type exposes both lower_bound and upper_bound.
template<typename _Type>
concept range_lookup_set_container =
    set_class<_Type>::has_range_lookup;

// full_lookup_set_container
//   concept: the type exposes find, count, and equal_range.
template<typename _Type>
concept full_lookup_set_container =
    set_class<_Type>::has_full_lookup;

// insertable_set_container
//   concept: the type exposes insert(value_type).
template<typename _Type>
concept insertable_set_container =
    has_set_insert_v<_Type>;

// emplaceable_set_container
//   concept: the type exposes emplace(...).
template<typename _Type>
concept emplaceable_set_container =
    has_set_emplace_v<_Type>;

// key_erasable_set_container
//   concept: the type exposes erase(key_type).
template<typename _Type>
concept key_erasable_set_container =
    has_set_erase_key_v<_Type>;

// extractable_set_container
//   concept: the type exposes extract(key_type).
template<typename _Type>
concept extractable_set_container =
    has_set_extract_v<_Type>;

// mergeable_set_container
//   concept: the type exposes merge(other).
template<typename _Type>
concept mergeable_set_container =
    has_set_merge_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
///              IV.  STRATEGY-BASED CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// contains_lookup_set_container
//   concept: lookup strategy prefers contains().
template<typename _Type>
concept contains_lookup_set_container =
    ( set_class<_Type>::lookup_strategy ==
      set_lookup_strategy::contains );

// find_lookup_set_container
//   concept: lookup strategy prefers find().
template<typename _Type>
concept find_lookup_set_container =
    ( set_class<_Type>::lookup_strategy ==
      set_lookup_strategy::find );

// count_lookup_set_container
//   concept: lookup strategy prefers count().
template<typename _Type>
concept count_lookup_set_container =
    ( set_class<_Type>::lookup_strategy ==
      set_lookup_strategy::count );

// linear_lookup_set_container
//   concept: lookup strategy falls back to linear scanning.
template<typename _Type>
concept linear_lookup_set_container =
    ( set_class<_Type>::lookup_strategy ==
      set_lookup_strategy::linear );

// emplacing_set_container
//   concept: insertion strategy prefers emplace().
template<typename _Type>
concept emplacing_set_container =
    ( set_class<_Type>::insert_strategy ==
      set_insert_strategy::emplace );

// inserting_set_container
//   concept: insertion strategy prefers insert(value_type).
template<typename _Type>
concept inserting_set_container =
    ( set_class<_Type>::insert_strategy ==
      set_insert_strategy::insert );

// key_erase_strategy_set_container
//   concept: erasure strategy prefers erase(key_type).
template<typename _Type>
concept key_erase_strategy_set_container =
    ( set_class<_Type>::erase_strategy ==
      set_erase_strategy::erase_key );

// iterator_erase_strategy_set_container
//   concept: erasure strategy uses erase(iterator) after lookup.
template<typename _Type>
concept iterator_erase_strategy_set_container =
    ( set_class<_Type>::erase_strategy ==
      set_erase_strategy::erase_iterator );


///////////////////////////////////////////////////////////////////////////////
///              V.   AGGREGATE CAPABILITY CONCEPTS                         ///
///////////////////////////////////////////////////////////////////////////////

// readable_set_container
//   concept: the type has a supported lookup strategy.
template<typename _Type>
concept readable_set_container =
    set_class<_Type>::is_readable;

// writable_set_container
//   concept: the type has a supported insertion strategy.
template<typename _Type>
concept writable_set_container =
    set_class<_Type>::is_writable;

// erasable_set_container
//   concept: the type has a supported erasure strategy.
template<typename _Type>
concept erasable_set_container =
    set_class<_Type>::is_erasable;

// fully_mutable_set_container
//   concept: the type supports both insertion and erasure.
template<typename _Type>
concept fully_mutable_set_container =
    set_class<_Type>::is_fully_mutable;

// classified_set_container
//   concept: shorthand for any type recognized by set_class.
template<typename _Type>
concept classified_set_container =
    set_class<_Type>::is_set;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_SET_CONCEPTS_