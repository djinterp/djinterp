/******************************************************************************
* djinterp [container] observational_comparison_concepts.hpp C++20 concepts for
* the OBSERVATIONAL COMPARISON axis -- the `requires`-facing view of
* observational_comparison_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is
* exactly its trait, spelled so it can constrain a template instead of gating
* one through enable_if. The trait stays the single source of truth. NAMES.
* Where the obvious name is taken by a CONTAINER CLASS in this namespace, the
* concept takes an adjective form instead. A concept and a class of the same
* name in one namespace is a hard redeclaration, and this framework has already
* been bitten by that three times. PORTABILITY: Gated on C++20 + concepts. Below
* that the header is empty and callers use the `::value` / `_v` forms directly.
* path:
* /inc/djinterp/core/container/concepts/observational_comparison_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_OBSERVATIONAL_COMPARISON_CONCEPTS_
#define DJINTERP_OBSERVATIONAL_COMPARISON_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../traits/observational_comparison_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT CAN BE OBSERVED
// ==========================================================================


// SizeObservable
// concept: exposes size() -- the weakest observation, and value-free: knowing
// HOW MANY is not observing any element.
D_CONCEPT_FROM_TRAIT(SizeObservable, has_size_member_v)


// MembershipObservable
//   concept: exposes contains() -- membership without extraction.
D_CONCEPT_FROM_TRAIT(MembershipObservable, has_contains_member_v)


// CountObservable
//   concept: exposes count() -- multiplicity without extraction.
D_CONCEPT_FROM_TRAIT(CountObservable, has_count_member_v)


// IndexObservable
//   concept: exposes indexed access -- the observation that distinguishes a
// positional container from a bag.
D_CONCEPT_FROM_TRAIT(IndexObservable, has_indexed_access_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_OBSERVATIONAL_COMPARISON_CONCEPTS_
