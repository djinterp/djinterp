/******************************************************************************
* djinterp [sync]                                concurrency_strategy_tags.hpp
*
*   Concurrency-strategy tag types AND the concurrency-signature
* vocabulary for the trait classification system.
*   The tag aliases (section I) are consulted by the "fast path" of
* concurrency_strategy_traits.hpp.  A type that wishes to declare
* its synchronization strategy exposes
*     using concurrency_strategy_tag = locked_strategy_tag;
* and the trait system reads it directly - no structural probing.
*   The signature enums (section II) name the monograph's concurrency
* coordinates - progress grade, arity, iteration overlap-semantics,
* and reclamation obligation.  They live here, not in the trait
* header, so the lower mechanism layer (threadsafe_container_traits)
* can name a progress grade without a cycle back to the strategy
* layer that consumes them.
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
* path:      /inc/djinterp/core/sync/concurrency_strategy_tags.hpp
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


// =========================================================================
// II.  Concurrency Signature Vocabulary  (monograph "Concurrency")
// =========================================================================
//   The monograph fixes a container's place on the concurrency (access)
// axis as a PAIR (progress grade, arity), together with the ITERATION
// overlap-semantics it offers and the RECLAMATION discipline it owes.  The
// four enums below name those coordinates.
//
//   They live HERE, beside the strategy tags, for a layering reason: BOTH
// the mechanism layer (threadsafe_container_traits.hpp, which maps a lock
// policy to a progress grade) and the strategy layer
// (concurrency_strategy_traits.hpp, which maps a strategy to a full
// signature) must name them, and the mechanism layer cannot include the
// strategy layer without a cycle.  This lightweight, dependency-free header
// is the common ancestor both already reach.
//
//   Only the VOCABULARY is here.  The strategy -> signature MAPPING, and the
// `concurrency_signature` aggregate that bundles these four, belong with the
// strategy classification and live in concurrency_strategy_traits.hpp.
//
//   C++11+ only (scoped enums); the tag structs above remain usable in
// C++98.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// concurrency_progress
//   enum: the progress grade - whether agents are guaranteed to make
// headway.  A total order:
//     sequential < blocking < lock_free < wait_free
enum class concurrency_progress
{
    sequential = 0,   // safe only under a single agent (no overlap)
    blocking   = 1,   // an agent may wait for another (a held lock)
    lock_free  = 2,   // SOME operation always completes (no global stall)
    wait_free  = 3    // EVERY operation completes in bounded own-steps
};

// concurrency_arity
//   enum: how many agents may act at once on each side.  A total order:
//     swmr < mwmr
// - the concurrent reading of Mutability's own reader/writer split.
enum class concurrency_arity
{
    swmr = 0,         // single-writer / multi-reader
    mwmr = 1          // multi-writer  / multi-reader
};

// iteration_consistency
//   enum: the overlap-semantics of a traversal that runs concurrently with
// another agent's mutation - the third dimension the monograph adds to the
// Iterability grid of {const,non-const} x {compile-time,runtime}.
enum class iteration_consistency
{
    inconsistent      = 0,   // the non-concurrent degenerate (unsafe overlap)
    weakly_consistent = 1,   // sees a valid-but-shifting subset during overlap
    snapshot          = 2    // sees one frozen value for the whole traversal
};

// reclamation_obligation
//   enum: the safe-reclamation discipline a concurrent container owes when
// its cells are freed during execution.  Under concurrency "unreachable from
// the roots" no longer implies "unreachable by every agent", so a cell may
// be returned only when NO agent can still reach it - not merely when it is
// unlinked.
enum class reclamation_obligation
{
    none            = 0,   // nothing to reclaim (inline cells / immutable)
    individual      = 1,   // free on unlink; sound ONLY under exclusion or a
                           // single agent - unsound for shared nodes freed
                           // lock-free (the monograph's individual strategy)
    reference_count = 2,   // per-node / per-version count; free at zero
    grace_period    = 3,   // defer until every pre-existing reader has quiesced
    hazard          = 4,   // free only when no per-agent hazard slot names it
    collector       = 5    // a tracing collector never frees a reachable node
};

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_