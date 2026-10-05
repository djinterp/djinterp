/*******************************************************************************
* djinterp [core]                                 memory_discipline_concepts.hpp
*
* C++20 concepts for the MEMORY-DISCIPLINE vocabulary -- the `requires`-facing
* view of {traits}. THE CONCEPTS ADD NO POLICY. Each is exactly its trait,
* spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth. NAMES.
* meta/concepts.hpp already owns the general type-level concepts (the `_c`
* family), and constexpr_iterator_concepts.hpp the constexpr-iteration ones;
* neither is duplicated here. Where an obvious name is otherwise taken, the
* concept takes a form that cannot collide -- a concept and a class of one name
* in one namespace is a hard redeclaration. PORTABILITY: Gated on C++20 +
* concepts. Below that the header is empty and callers use the `::value` / `_v`
*
*
* path:      /inc/djinterp/core/meta/concepts/memory_discipline_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_CONCEPTS_MEMORY_DISCIPLINE_CONCEPTS_HPP
#define DJINTERP_META_CONCEPTS_MEMORY_DISCIPLINE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../type_utility.hpp"  // clean_t
#include "../memory_discipline.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE STRATEGY SIGNALS  (what a strategy type advertises)
// ==========================================================================


// DeclaresStrategyKind
// concept: advertises a strategy_kind -- the tell that a type is a memory
// strategy at all, the guard the discipline classifier rests on.
template<typename Type>
concept DeclaresStrategyKind =
    has_strategy_kind_signal_v<clean_t<Type>>;


// ByteAllocatingStrategy
//   concept: allocates by BYTES -- an untyped region (arena / bump).
template<typename Type>
concept ByteAllocatingStrategy =
    has_byte_allocate_signal_v<clean_t<Type>>;


// ElementAllocatingStrategy
//   concept: allocates by ELEMENTS -- a typed slot source (pool / individual).
template<typename Type>
concept ElementAllocatingStrategy =
    has_element_allocate_signal_v<clean_t<Type>>;


// ==========================================================================
//  THE POOL/HEAP DISCRIMINATOR  (stability, not a type name)
// ==========================================================================


// DeclaresPointerStability
// concept: states whether its slots move. Pointer stability is exactly what
// tells a pool from a heap -- both release per object -- so this constant is
// the discriminator, named by contract rather than by any concrete pool type.
template<typename Type>
concept DeclaresPointerStability =
    has_pointer_stable_constant_signal_v<clean_t<Type>>;


// DeclaresIndividualRelease
// concept: states whether it releases per object. With stability, this is what
// separates pooled from arena: an arena frees all at once, a pool frees one
// slot at a time.
template<typename Type>
concept DeclaresIndividualRelease =
    has_individual_release_constant_signal_v<clean_t<Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now


#endif  // DJINTERP_META_CONCEPTS_MEMORY_DISCIPLINE_CONCEPTS_HPP
