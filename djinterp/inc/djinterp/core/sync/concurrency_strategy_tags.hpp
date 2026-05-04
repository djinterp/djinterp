/******************************************************************************
* djinterp [sync]                                concurrency_concurrency_strategy_tags.hpp
*
*   Concurrency-strategy tag types for the trait classification system.
*   These are the tag aliases consulted by the "fast path" of
* concurrency_strategy_traits.hpp.  A type that wishes to declare
* its synchronization strategy exposes
*     using concurrency_strategy_tag = locked_strategy_tag;
* and the trait system reads it directly - no structural probing.
*
*   This header is intentionally lightweight: it has NO dependencies
* beyond djinterp.hpp itself.  Production-side primitives in
* /sync (atomic, cow, rcu, hazard_pointer) and /container
* (threadsafe_container_base) include this header to declare their
* tag without taking a transitive dependency on
* concurrency_strategy_traits.hpp (which would create a cycle, since
* the trait header consumes the strategies).
*
*   The tag definitions previously lived inside
* concurrency_strategy_traits.hpp.  They have been factored out so
* that the foundation primitives can self-tag without depending on
* the trait header.  concurrency_strategy_traits.hpp now includes
* this file and the tag names remain visible in traits
* from the trait header for all existing call sites.
*
*
* path:      /inc/djinterp/core/sync/concurrency_concurrency_strategy_tags.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.02
******************************************************************************/

#ifndef DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_
#define DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_ 1

#ifndef __cplusplus
    #error "concurrency_strategy_tags.hpp requires C++ compilation"
#endif

// djinterp
#include "../djinterp.hpp"


NS_DJINTERP

// =========================================================================
// I.   STRATEGY TAG TYPES
// =========================================================================
//   Empty tag types used by the trait system to classify
// synchronization strategies.  Foundation primitives expose
// `using concurrency_strategy_tag = <name>;` to declare
// their strategy.

// none_strategy_tag
//   tag: type uses no synchronization (single-threaded
// or external).
struct none_strategy_tag   {};

// locked_strategy_tag
//   tag: type uses mutex / rwlock synchronization under a
// lock policy.
struct locked_strategy_tag {};

// cow_strategy_tag
//   tag: type uses copy-on-write with snapshot handles.
struct cow_strategy_tag    {};

// rcu_strategy_tag
//   tag: type uses read-copy-update / epoch-based
// reclamation.
struct rcu_strategy_tag    {};

// atomic_strategy_tag
//   tag: type is lock-free using std::atomic<T> for the
// underlying state.
struct atomic_strategy_tag {};

// hazard_strategy_tag
//   tag: type uses hazard-pointer-protected lock-free
// pointer chasing.
struct hazard_strategy_tag {};

// hybrid_strategy_tag
//   tag: type combines two or more strategies (e.g.
// locked metadata around a cow payload).
struct hybrid_strategy_tag {};


NS_END  // djinterp


#endif  // DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_