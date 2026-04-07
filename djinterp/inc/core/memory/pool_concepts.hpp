/******************************************************************************
* djinterp [container]                                       pool_concepts.hpp
*
*  djinterp pool classification concepts
*   C++20 concepts layered on top of pool_traits.hpp.  These concepts
* provide readable `requires` constraints for pool resources,
* pool-backed allocators, and containers using such allocators.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait or
* variable template from the pool trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Pool Resource Concepts
* 3.   Pool Stability and Release Concepts
* 4.   Pool Capability Concepts
* 5.   Allocator and Container Concepts
* 6.   Resource Extraction Concepts
*
* 
* path:      /inc/djinterp/pool/pool_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                   date: 2026.04.01
******************************************************************************/

#ifndef DJINTERP_CONTAINER_POOL_CONCEPTS_
#define DJINTERP_CONTAINER_POOL_CONCEPTS_ 1

#include <type_traits>
#include "pool_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "pool_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Pool Resource Concepts
// =============================================================================

// pool_resource
//   concept: constrains types satisfying the minimum pool resource protocol.
template<typename _Type>
concept pool_resource =
    is_pool_resource_v<clean_t<_Type>>;

// non_pool_resource
//   concept: constrains types that do not satisfy the pool resource protocol.
template<typename _Type>
concept non_pool_resource =
    !pool_resource<_Type>;

// acquiring_pool
//   concept: constrains pool-like types exposing acquire().
template<typename _Type>
concept acquiring_pool =
    has_acquire_v<clean_t<_Type>>;

// releasing_pool
//   concept: constrains pool-like types exposing release(void*).
template<typename _Type>
concept releasing_pool =
    has_release_v<clean_t<_Type>>;

// sized_pool
//   concept: constrains pool-like types exposing size().
template<typename _Type>
concept sized_pool =
    has_size_accessor_v<clean_t<_Type>>;

// typed_pool
//   concept: constrains pool-like types exposing value_type.
template<typename _Type>
concept typed_pool =
    has_value_type_v<clean_t<_Type>>;

// classified_pool
//   concept: constrains types recognized by the pool trait layer.
template<typename _Type>
concept classified_pool =
    ( pool_resource<_Type>                     ||
      is_pointer_stable_pool_v<clean_t<_Type>> ||
      supports_individual_release_v<clean_t<_Type>> ||
      has_generational_sweep_v<clean_t<_Type>> ||
      has_memory_accounting_v<clean_t<_Type>> );


// =============================================================================
// II.  Pool Stability and Release Concepts
// =============================================================================

// pointer_stable_pool
//   concept: constrains pools guaranteeing pointer stability across growth.
template<typename _Type>
concept pointer_stable_pool =
    is_pointer_stable_pool_v<clean_t<_Type>>;

// relocatable_pool
//   concept: constrains pools that do not guarantee pointer stability.
template<typename _Type>
concept relocatable_pool =
    ( pool_resource<_Type> &&
      !pointer_stable_pool<_Type> );

// individually_releasing_pool
//   concept: constrains pools supporting per-slot release.
template<typename _Type>
concept individually_releasing_pool =
    supports_individual_release_v<clean_t<_Type>>;

// monotonic_pool
//   concept: constrains pools reclaiming memory only on reset or destruction.
template<typename _Type>
concept monotonic_pool =
    is_monotonic_pool_v<clean_t<_Type>>;

// generational_pool
//   concept: constrains pools supporting generational reclamation.
template<typename _Type>
concept generational_pool =
    has_generational_sweep_v<clean_t<_Type>>;


// =============================================================================
// III. Pool Capability Concepts
// =============================================================================

// resettable_pool
//   concept: constrains pools exposing reset().
template<typename _Type>
concept resettable_pool =
    has_pool_reset_v<clean_t<_Type>>;

// reservable_pool
//   concept: constrains pools exposing reserve(size_t).
template<typename _Type>
concept reservable_pool =
    has_pool_reserve_v<clean_t<_Type>>;

// slot_sized_pool
//   concept: constrains pools exposing bytes_per_slot().
template<typename _Type>
concept slot_sized_pool =
    has_bytes_per_slot_v<clean_t<_Type>>;

// slot_aligned_pool
//   concept: constrains pools exposing alignment().
template<typename _Type>
concept slot_aligned_pool =
    has_slot_alignment_v<clean_t<_Type>>;

// generation_tracked_pool
//   concept: constrains pools exposing current_generation().
template<typename _Type>
concept generation_tracked_pool =
    has_current_generation_v<clean_t<_Type>>;

// generation_advancing_pool
//   concept: constrains pools exposing advance_generation().
template<typename _Type>
concept generation_advancing_pool =
    has_advance_generation_v<clean_t<_Type>>;

// memory_accounting_pool
//   concept: constrains pools exposing byte-level accounting.
template<typename _Type>
concept memory_accounting_pool =
    has_memory_accounting_v<clean_t<_Type>>;

// bytes_allocated_pool
//   concept: constrains pools exposing bytes_allocated().
template<typename _Type>
concept bytes_allocated_pool =
    has_bytes_allocated_v<clean_t<_Type>>;

// bytes_in_use_pool
//   concept: constrains pools exposing bytes_in_use().
template<typename _Type>
concept bytes_in_use_pool =
    has_bytes_in_use_v<clean_t<_Type>>;

// utilization_reporting_pool
//   concept: constrains pools exposing utilization().
template<typename _Type>
concept utilization_reporting_pool =
    has_utilization_v<clean_t<_Type>>;


// =============================================================================
// IV.  Allocator and Container Concepts
// =============================================================================

// pool_allocator
//   concept: constrains allocators backed by a pool resource.
template<typename _Type>
concept pool_allocator =
    is_pool_allocator_v<clean_t<_Type>>;

// non_pool_allocator
//   concept: constrains allocators not backed by a pool resource.
template<typename _Type>
concept non_pool_allocator =
    !pool_allocator<_Type>;

// resource_exposing_pool_allocator
//   concept: constrains pool allocators exposing resource().
template<typename _Type>
concept resource_exposing_pool_allocator =
    has_resource_method_v<clean_t<_Type>>;

// pool_backed_container
//   concept: constrains containers whose allocator is pool-backed.
template<typename _Type>
concept pool_backed_container =
    is_pool_backed_container_v<clean_t<_Type>>;

// non_pool_backed_container
//   concept: constrains containers whose allocator is not pool-backed.
template<typename _Type>
concept non_pool_backed_container =
    !pool_backed_container<_Type>;

// allocator_aware_pool_container
//   concept: constrains pool-backed containers exposing allocator_type.
template<typename _Type>
concept allocator_aware_pool_container =
    ( pool_backed_container<_Type> &&
      has_allocator_type_v<clean_t<_Type>> );


// =============================================================================
// V.   Resource Extraction Concepts
// =============================================================================

// pool_allocator_with_resource
//   concept: constrains pool allocators whose resource type can be extracted.
template<typename _Type>
concept pool_allocator_with_resource =
    ( pool_allocator<_Type> &&
      !std::is_void_v<pool_resource_type_t<clean_t<_Type>>> );

// stable_pool_allocator
//   concept: constrains pool allocators backed by pointer-stable pools.
template<typename _Type>
concept stable_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      is_pointer_stable_pool_v<
          pool_resource_type_t<clean_t<_Type>>> );

// monotonic_pool_allocator
//   concept: constrains pool allocators backed by monotonic pools.
template<typename _Type>
concept monotonic_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      is_monotonic_pool_v<
          pool_resource_type_t<clean_t<_Type>>> );

// generational_pool_allocator
//   concept: constrains pool allocators backed by generational pools.
template<typename _Type>
concept generational_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      has_generational_sweep_v<
          pool_resource_type_t<clean_t<_Type>>> );


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_POOL_CONCEPTS_
