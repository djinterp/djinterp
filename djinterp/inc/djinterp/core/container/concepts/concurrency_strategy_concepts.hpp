/******************************************************************************
* djinterp [container] concurrency_strategy_concepts.hpp C++20 concepts for the
* CONCURRENCY STRATEGY axis -- the `requires`-facing view of
* concurrency_strategy_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth; if a
* classification is wrong, it is wrong in one place. That is the whole point of
* generating these rather than restating the detection logic in `requires`
* clauses. PORTABILITY: Gated on C++20 + concepts. Below that the header is
* empty and callers use the `::value` / `_v` forms directly -- which is why
* nothing else in the framework is allowed to depend on these. path:
* /inc/djinterp/core/container/concepts/concurrency_strategy_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONCURRENCY_STRATEGY_CONCEPTS_
#define DJINTERP_CONCURRENCY_STRATEGY_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/concurrency_strategy_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE TWO ENDS
// ==========================================================================


// concurrent_container
// concept: safe under concurrent use by SOME strategy -- the umbrella the six
// below refine.
template<typename _Type>
concept concurrent_container =
    is_concurrent_container_v<clean_t<_Type>>;


// sequentially_accessed_container
// concept: NO concurrency strategy: single-threaded use only. Named this way,
// not `sequential_container`, because that name is already a CONTAINER in this
// framework (sequential_container.hpp) and a concept sharing it would collide
// in the same namespace.
template<typename _Type>
concept sequentially_accessed_container =
    is_sequential_container_v<clean_t<_Type>>;


// ==========================================================================
//  THE STRATEGIES
// ==========================================================================


// locked_container
//   concept: guarded by a lock -- it exposes read_lock() / write_lock().
template<typename _Type>
concept locked_container =
    is_locked_container_v<clean_t<_Type>>;


// atomic_container
//   concept: element access is atomic -- no lock, no snapshot.
template<typename _Type>
concept atomic_container =
    is_atomic_container_v<clean_t<_Type>>;


// cow_container
//   concept: copy-on-write: readers see an immutable snapshot, writers copy.
template<typename _Type>
concept cow_container =
    is_cow_container_v<clean_t<_Type>>;


// rcu_container
// concept: read-copy-update: readers never block, reclamation waits for a grace
// period.
template<typename _Type>
concept rcu_container =
    is_rcu_container_v<clean_t<_Type>>;


// hazard_container
// concept: hazard-pointer protected: readers publish what they are holding so
// it is not reclaimed under them.
template<typename _Type>
concept hazard_container =
    is_hazard_container_v<clean_t<_Type>>;


// ==========================================================================
//  EDGES
// ==========================================================================


// synchronized_container
//   concept: synchronized by SOME mechanism -- locked, atomic, or otherwise.
template<typename _Type>
concept synchronized_container =
    is_synchronized_container_v<clean_t<_Type>>;


// vacuously_concurrent_container
//   concept: safe under concurrency because there is nothing to race ON -- an
// immutable container needs no strategy, and saying it 'has' one would be
// a category error. This is why is_concurrent is not simply 'declares a
// strategy'.
template<typename _Type>
concept vacuously_concurrent_container =
    is_vacuously_concurrent_v<clean_t<_Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONCURRENCY_STRATEGY_CONCEPTS_
