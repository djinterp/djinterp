/******************************************************************************
* djinterp [container] container_multiplicity_concepts.hpp C++20 concepts for
* the MULTIPLICITY axis -- the `requires`-facing view of
* container_multiplicity_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth. NAMES. Where
* the obvious name is taken by a CONTAINER CLASS in this namespace, the concept
* takes an adjective form instead. A concept and a class of the same name in one
* namespace is a hard redeclaration, and this framework has already been bitten
* by that three times. PORTABILITY: Gated on C++20 + concepts. Below that the
* header is empty and callers use the `::value` / `_v` forms directly. path:
* /inc/djinterp/core/container/concepts/container_multiplicity_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_
#define DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/container_multiplicity_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE OCCURRENCE BOUND m
// ==========================================================================


// UniqueContainer
//   concept: one occurrence per equivalence class, m = 1 -- set semantics.
D_CONCEPT_FROM_TRAIT(UniqueContainer, is_unique_container_v)


// MultisetContainer
//   concept: a genuine equivalence with m > 1 -- duplicates admitted.
D_CONCEPT_FROM_TRAIT(MultisetContainer, is_multiset_container_v)


// SequenceMultiplicityContainer
//   concept: comparator-less: identity equivalence, m = infinity, copies
// distinguished by position. Named in PascalCase; every concept is, so a
// concept and a same-named snake_case class cannot collide and the natural name
// is safe.
D_CONCEPT_FROM_TRAIT(SequenceMultiplicityContainer, is_sequence_container_v)


// ==========================================================================
//  SIGNALS
// ==========================================================================


// CountableContainer
//   concept: the axis guard -- a value_type and a const-callable size().
D_CONCEPT_FROM_TRAIT(CountableContainer, is_countable_container_v)


// DeclaresMultiplicityBound
// concept: carries the opt-in static `multiplicity` -- the authoritative
// override, and the only way to state a bound with no structural tell (a
// bounded multiset, or a keyed container whose insert takes a key and a value
// rather than one value_type).
D_CONCEPT_FROM_TRAIT(DeclaresMultiplicityBound, has_multiplicity_bound_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_
