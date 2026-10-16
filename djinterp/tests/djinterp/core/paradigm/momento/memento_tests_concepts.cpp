/*******************************************************************************
* djinterp [core]                                     memento_tests_concepts.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_concepts.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_concepts.cpp
//   The concept-constrained interfaces (section IX, C++20+): memento_source,
// memento_target, snapshot_strategy, history_policy, and constrained_checkpoint.
// Every predicate body is guarded on D_MEMENTO_HAS_CONCEPTS and passes vacuously
// on older dialects, so the declarations still link at C++11.

// std
#include <vector>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_memento_source_concept
  Verifies the memento_source concept.
  Tests the following:
  - satisfied by a save_state/restore_state type (sr_state)
  - satisfied by a merely copy-constructible type (plain_state)
  - not satisfied by a non-copyable type lacking the protocol
*/
bool
tests_memento_source_concept()
{
#if D_MEMENTO_HAS_CONCEPTS
    struct noncopy_noproto
    {
        noncopy_noproto() = default;
        noncopy_noproto(const noncopy_noproto&) = delete;
    };

    static_assert( memento_source<sr_state>,       "protocol source");
    static_assert( memento_source<plain_state>,    "copyable source");
    static_assert(!memento_source<noncopy_noproto>,"neither");

    return ( memento_source<sr_state> &&
             memento_source<plain_state> &&
            !memento_source<noncopy_noproto> );
#else
    return true;
#endif
}

/*
tests_memento_target_concept
  Verifies the memento_target concept.
  Tests the following:
  - satisfied by a copy-assignable type (int)
  - satisfied by a save_state/restore_state type (sr_state)
*/
bool
tests_memento_target_concept()
{
#if D_MEMENTO_HAS_CONCEPTS
    static_assert(memento_target<int>,      "assignable target");
    static_assert(memento_target<sr_state>, "restorable target");

    return (memento_target<int> && memento_target<sr_state>);
#else
    return true;
#endif
}

/*
tests_snapshot_strategy_concept
  Verifies the snapshot_strategy concept.
  Tests the following:
  - satisfied by deep_copy_snapshot for a copyable state
  - satisfied by clone_snapshot for a clonable state
*/
bool
tests_snapshot_strategy_concept()
{
#if D_MEMENTO_HAS_CONCEPTS
    static_assert(snapshot_strategy<deep_copy_snapshot, int>, "deep-copy");
    static_assert(snapshot_strategy<clone_snapshot, clonable_state>, "clone");

    return ( snapshot_strategy<deep_copy_snapshot, int> &&
             snapshot_strategy<clone_snapshot, clonable_state> );
#else
    return true;
#endif
}

/*
tests_history_policy_concept
  Verifies the history_policy concept.
  Tests the following:
  - satisfied by unlimited_history and bounded_history over a memento container
*/
bool
tests_history_policy_concept()
{
#if D_MEMENTO_HAS_CONCEPTS
    using container = std::vector<memento<int> >;

    static_assert(history_policy<unlimited_history, container, int>, "unlimited");
    static_assert(history_policy<bounded_history<4>, container, int>, "bounded");

    return ( history_policy<unlimited_history, container, int> &&
             history_policy<bounded_history<4>, container, int> );
#else
    return true;
#endif
}

/*
tests_constrained_checkpoint
  Verifies constrained_checkpoint produces a memento under its constraints.
  Tests the following:
  - a checkpoint of a copyable state yields a memento holding that state
*/
bool
tests_constrained_checkpoint()
{
#if D_MEMENTO_HAS_CONCEPTS
    auto m = constrained_checkpoint(123);

    return (m.state == 123);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
