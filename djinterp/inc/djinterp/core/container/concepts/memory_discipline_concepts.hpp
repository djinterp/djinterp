/******************************************************************************
* djinterp [container]                           memory_discipline_concepts.hpp
*
* C++20 concepts for the MEMORY-DISCIPLINE vocabulary -- the `requires`-facing
* view of meta/memory_discipline.hpp. THE CONCEPTS ADD NO POLICY. Each is
* exactly its trait,
* spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth. NAMES.
* meta/concepts.hpp already owns the general type-level concepts (the `_c`
* family), and constexpr_iterator_concepts.hpp the constexpr-iteration ones;
* neither is duplicated here. Where an obvious name is otherwise taken, the
* concept takes a form that cannot collide -- a concept and a class of one name
* in one namespace is a hard redeclaration. PORTABILITY: Gated on C++20 +
* concepts. Below that the header is empty and callers use the `::value` / `_v`
* forms directly. path: /inc/djinterp/core/meta/memory_discipline_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_META_MEMORY_DISCIPLINE_CONCEPTS_
#define DJINTERP_META_MEMORY_DISCIPLINE_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../../meta/memory_discipline.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE STRATEGY SIGNALS  (what a strategy type advertises)
// ==========================================================================


// DeclaresStrategyKind
// concept: advertises a strategy_kind -- the tell that a type is a memory
// strategy at all, the guard the discipline classifier rests on.
D_CONCEPT_FROM_TRAIT(DeclaresStrategyKind, has_strategy_kind_signal_v)


// ByteAllocatingStrategy
//   concept: allocates by BYTES -- an untyped region (arena / bump).
D_CONCEPT_FROM_TRAIT(ByteAllocatingStrategy, has_byte_allocate_signal_v)


// ElementAllocatingStrategy
//   concept: allocates by ELEMENTS -- a typed slot source (pool / individual).
D_CONCEPT_FROM_TRAIT(ElementAllocatingStrategy, has_element_allocate_signal_v)


// ==========================================================================
//  THE POOL/HEAP DISCRIMINATOR  (stability, not a type name)
// ==========================================================================


// DeclaresPointerStability
// concept: states whether its slots move. Pointer stability is exactly what
// tells a pool from a heap -- both release per object -- so this constant is
// the discriminator, named by contract rather than by any concrete pool type.
D_CONCEPT_FROM_TRAIT(DeclaresPointerStability,
                     has_pointer_stable_constant_signal_v)


// DeclaresIndividualRelease
// concept: states whether it releases per object. With stability, this is what
// separates pooled from arena: an arena frees all at once, a pool frees one
// slot at a time.
D_CONCEPT_FROM_TRAIT(DeclaresIndividualRelease,
                     has_individual_release_constant_signal_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_META_MEMORY_DISCIPLINE_CONCEPTS_
