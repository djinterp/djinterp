/*******************************************************************************
* djinterp [core]                                  container_memory_concepts.hpp
*
* C++20 concepts for the MEMORY DISCIPLINE axis -- the `requires`-facing view
* of container_memory_traits.hpp.
*
*   THE CONCEPTS ADD NO POLICY.
*   Each is exactly its trait, spelled so it can constrain a template instead
* of gating one through enable_if. The trait stays the single source of truth;
* if a classification is wrong, it is wrong in one place. That is the whole
* point of generating these rather than restating the detection logic in
* `requires` clauses.
*
*   PORTABILITY:
*   Gated on C++20 + concepts. Below that the header is empty and callers use
* the `::value` / `_v` forms directly.
*
*
* path:      /inc/djinterp/core/container/concepts/container_memory_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MEMORY_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MEMORY_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_memory_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  DISCIPLINE
// ==========================================================================


// inline_storage_container
//   concept: allocates nothing -- its cells are embedded in its own footprint
// (the NONE discipline).
template<typename Type>
concept inline_storage_container =
    container_has_inline_storage_v<clean_t<Type>>;


// heap_container
//   concept: draws on a general-purpose, per-object, NON-pointer-stable heap
// (the INDIVIDUAL discipline). Every standard allocator-aware container is
// this.
template<typename Type>
concept heap_container =
    container_uses_individual_heap_v<clean_t<Type>>;


// pooled_container
//   concept: draws on a POINTER-STABLE, individually-releasing pool. Stability
// is what tells a pool from a heap -- not the name of any concrete type.
template<typename Type>
concept pooled_container =
    container_uses_pool_v<clean_t<Type>>;


// arena_container
//   concept: draws on an arena / region -- a bump that may run over inline or
// heap, and so constrains the siting not at all.
template<typename Type>
concept arena_container =
    container_uses_arena_v<clean_t<Type>>;


// ==========================================================================
//  PREDICATES
// ==========================================================================


// allocating_container
//   concept: acquires storage at runtime at all -- individual, pooled, OR
// arena.
template<typename Type>
concept allocating_container =
    container_allocates_v<clean_t<Type>>;


// memory_resolved_container
//   concept: its discipline was actually determined. Constrain on this before
// branching on the others, or `unknown` will silently take a branch.
template<typename Type>
concept memory_resolved_container =
    container_memory_resolved_v<clean_t<Type>>;


// memory_consistent_container
//   concept: its DISCIPLINE and its independently-classified SITING agree. A
// no- allocation discipline must be static storage; an allocating one must be
// dynamic. This is the cross-axis check, as a constraint.
template<typename Type>
concept memory_consistent_container =
    container_memory_consistent_v<clean_t<Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MEMORY_CONCEPTS_HPP
