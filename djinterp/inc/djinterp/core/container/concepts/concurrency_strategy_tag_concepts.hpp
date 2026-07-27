/******************************************************************************
* djinterp [container]                     concurrency_strategy_tag_concepts.hpp
*
* C++20 concepts for the CONCURRENCY-STRATEGY tag / protection contracts -- the
* `requires`-facing view of sync/concurrency_strategy_tags.hpp. THE CONCEPTS
* ADD NO POLICY. Each is
* exactly its trait, spelled so it can constrain a template instead of gating
* one through enable_if. The trait stays the single source of truth. NAMES.
* meta/concepts.hpp already owns the general type-level concepts (the `_c`
* family), and constexpr_iterator_concepts.hpp the constexpr-iteration ones;
* neither is duplicated here. Where an obvious name is otherwise taken, the
* concept takes a form that cannot collide -- a concept and a class of one name
* in one namespace is a hard redeclaration. PORTABILITY: Gated on C++20 +
* concepts. Below that the header is empty and callers use the `::value` / `_v`
* forms directly. path:
* /inc/djinterp/core/sync/concurrency_strategy_tag_concepts.hpp link(s): TBA
* author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONCURRENCY_STRATEGY_TAG_CONCEPTS_
#define DJINTERP_CONCURRENCY_STRATEGY_TAG_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_HAS_TYPE
#include "../../sync/concurrency_strategy_tags.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  SELF-DECLARED STRATEGY
// ==========================================================================


// StrategyTagged
// concept: carries a concurrency_strategy_tag -- it names its own
// synchronization strategy, and the trait system reads it directly instead of
// probing structurally. This is the fast path; a type that does not tag is
// classified by what it exposes.
D_CONCEPT_HAS_TYPE(StrategyTagged, concurrency_strategy_tag)


// ==========================================================================
//  PROTECTION CONTRACTS  (the reclamation a strategy owes)
// ==========================================================================


// COWProtected
// concept: exposes a cow_state_type -- copy-on-write: readers hold an immutable
// snapshot, writers copy. The reclamation obligation is the snapshot's own
// lifetime.
D_CONCEPT_HAS_TYPE(COWProtected, cow_state_type)


// RCUProtected
// concept: exposes an rcu_protected_type -- read-copy-update: readers never
// block, and a freed cell waits for a grace period, because under concurrency
// 'unlinked' does not imply 'unreachable by every agent'.
D_CONCEPT_HAS_TYPE(RCUProtected, rcu_protected_type)


// HazardProtected
// concept: exposes a hazard_domain_type -- hazard pointers: a reader publishes
// what it holds so that cell is not reclaimed under it. A different answer to
// the same reclamation question RCU answers with epochs.
D_CONCEPT_HAS_TYPE(HazardProtected, hazard_domain_type)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONCURRENCY_STRATEGY_TAG_CONCEPTS_
