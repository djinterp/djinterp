/******************************************************************************
* djinterp [memory]                               memory_strategy_concepts.hpp
*
*  djinterp memory-strategy classification concepts  (CORE)
*   C++20 concepts layered on top of memory_strategy_traits.hpp.  Strategy- and
* container-agnostic: these concepts name no concrete provider and reason about
* no container.  Each forwards to the corresponding public trait or `_v`
* variable template - no detection is re-implemented here (mirrors the pattern
* of pool_concepts.hpp).
*
*
* path:      /inc/djinterp/core/memory/memory_strategy_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.   feature gate
2.   membership concepts
3.   typing concepts
4.   storage-discipline concepts
5.   stability and release concepts
*/

#ifndef DJINTERP_MEMORY_STRATEGY_CONCEPTS_
#define DJINTERP_MEMORY_STRATEGY_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./memory_strategy_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "memory_strategy_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP


// ===========================================================================
// I.   Membership Concepts
// ===========================================================================

// memory_strategy
//   concept: minimum (descriptive) contract - declares a storage discipline.
template<typename _Type>
concept memory_strategy = is_memory_strategy_v<_Type>;

// non_memory_strategy
template<typename _Type>
concept non_memory_strategy = !memory_strategy<_Type>;

// allocating_strategy
//   concept: also supplies and reclaims storage at runtime (either typing).
template<typename _Type>
concept allocating_strategy = is_allocating_strategy_v<_Type>;

// descriptive_only_strategy
//   concept: declares discipline but does not allocate (static / in-object).
template<typename _Type>
concept descriptive_only_strategy = is_descriptive_only_strategy_v<_Type>;


// ===========================================================================
// II.  Typing Concepts
// ===========================================================================

// element_strategy
//   concept: element-typed surface (value_type + allocate(n)/deallocate(p,n)).
template<typename _Type>
concept element_strategy = is_element_strategy_v<_Type>;

// byte_strategy
//   concept: byte-typed surface (allocate(bytes,align)/deallocate(p,bytes,align)).
template<typename _Type>
concept byte_strategy = is_byte_strategy_v<_Type>;

// allocating_element_strategy / allocating_byte_strategy
//   concept: the two operational typings, explicitly.
template<typename _Type>
concept allocating_element_strategy =
    ( allocating_strategy<_Type> && element_strategy<_Type> );

template<typename _Type>
concept allocating_byte_strategy =
    ( allocating_strategy<_Type> && byte_strategy<_Type> );


// ===========================================================================
// III. Storage-discipline Concepts
// ===========================================================================

// static_memory_strategy
template<typename _Type>
concept static_memory_strategy =
    ( memory_strategy<_Type> && is_static_strategy_v<_Type> );

// fixed_memory_strategy
template<typename _Type>
concept fixed_memory_strategy =
    ( memory_strategy<_Type> && is_fixed_strategy_v<_Type> );

// dynamic_memory_strategy
template<typename _Type>
concept dynamic_memory_strategy =
    ( memory_strategy<_Type> && is_dynamic_strategy_v<_Type> );


// ===========================================================================
// IV.  Stability and Release Concepts
// ===========================================================================

// pointer_stable_strategy
template<typename _Type>
concept pointer_stable_strategy =
    ( memory_strategy<_Type> && is_pointer_stable_strategy_v<_Type> );

// individually_releasing_strategy
template<typename _Type>
concept individually_releasing_strategy =
    ( allocating_strategy<_Type> &&
      supports_individual_release_strategy_v<_Type> );

// monotonic_strategy
template<typename _Type>
concept monotonic_strategy =
    ( allocating_strategy<_Type> && is_monotonic_strategy_v<_Type> );

// generational_strategy
template<typename _Type>
concept generational_strategy =
    ( allocating_strategy<_Type> && is_generational_strategy_v<_Type> );


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_STRATEGY_CONCEPTS_
