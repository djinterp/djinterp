/*******************************************************************************
* djinterp [test]                concurrency_strategy_traits_tests_aggregate.cpp
*
*   Section IV.b - the vacuous / concurrent / synchronized / sequential axis.
* These are tested as the header's own logical invariants so they hold whatever
* the real is_immutable_container decides, while still pinning the concrete
* true/false values that do not depend on immutability.  Defines the
* "aggregate" block.
*
*
* path:      /tests/djinterp/cpp/container/sync/concurrency_strategy_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// FLOOR, FOR NOW: below C++17, concurrency_strategy_traits.hpp's floor,
// this unit is empty, as that header is.
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_aggregate_synchronized
//   Verifies is_synchronized_container is exactly the disjunction of the five
//   strategy predicates - the "runs machinery" question.
//   Tests the following:
//   - true for each strategy-bearing fixture, false for the empty type
//   - equals (locked || cow || rcu || atomic || hazard) on mixed inputs
bool tests_aggregate_synchronized()
{
    D_CS_CHECK(is_synchronized_container_v<DStLockedRw>);
    D_CS_CHECK(is_synchronized_container_v<DStCowState>);
    D_CS_CHECK(is_synchronized_container_v<DStRcuProtected>);
    D_CS_CHECK(is_synchronized_container_v<st_atomic_value>);
    D_CS_CHECK(is_synchronized_container_v<DStHazard>);
    D_CS_CHECK(is_synchronized_container_v<DHyLockedCow>);
    D_CS_CHECK(!is_synchronized_container_v<DStPlain>);
    D_CS_CHECK(!is_synchronized_container_v<DImmutableProbe>);

    // the exact composition, on a hybrid input
    D_CS_CHECK(is_synchronized_container_v<DHyLockedCow> ==
               ( is_locked_container_v<DHyLockedCow> ||
                 is_cow_container_v<DHyLockedCow>    ||
                 is_rcu_container_v<DHyLockedCow>    ||
                 is_atomic_container_v<DHyLockedCow> ||
                 is_hazard_container_v<DHyLockedCow> ));
    return true;
}

// tests_aggregate_vacuous
//   Verifies is_vacuously_concurrent forwards is_immutable_container.  The
//   forwarding invariant holds regardless of the immutability model; the
//   DImmutableProbe additionally lights up the true arm under the reference
//   container_traits.
//   Tests the following:
//   - is_vacuously_concurrent_v<T> == is_immutable_container_v<T>
//   - a synchronized-but-mutable fixture is not vacuously concurrent
bool tests_aggregate_vacuous()
{
    D_CS_CHECK(is_vacuously_concurrent_v<DImmutableProbe> ==
               is_immutable_container_v<DImmutableProbe>);
    D_CS_CHECK(is_vacuously_concurrent_v<DStPlain> ==
               is_immutable_container_v<DStPlain>);
    D_CS_CHECK(is_vacuously_concurrent_v<DStCowState> ==
               is_immutable_container_v<DStCowState>);

    // a live-machinery fixture is not immutable, so not vacuously concurrent
    D_CS_CHECK(!is_vacuously_concurrent_v<DStLockedRw>);
    D_CS_CHECK(is_vacuously_concurrent_v<DImmutableProbe>);
    return true;
}

// tests_aggregate_concurrent
//   Verifies is_concurrent_container is synchronized OR vacuously concurrent -
//   the header's alignment with the monograph ("concurrency is not the same as
//   'has a lock'").
//   Tests the following:
//   - equals (synchronized || vacuously-concurrent) across inputs
//   - true for each strategy-bearing fixture
//   - the empty mutable type is not concurrent
bool tests_aggregate_concurrent()
{
    D_CS_CHECK(is_concurrent_container_v<DStLockedRw> ==
               ( is_synchronized_container_v<DStLockedRw> ||
                 is_vacuously_concurrent_v<DStLockedRw> ));
    D_CS_CHECK(is_concurrent_container_v<DImmutableProbe> ==
               ( is_synchronized_container_v<DImmutableProbe> ||
                 is_vacuously_concurrent_v<DImmutableProbe> ));
    D_CS_CHECK(is_concurrent_container_v<DStPlain> ==
               ( is_synchronized_container_v<DStPlain> ||
                 is_vacuously_concurrent_v<DStPlain> ));

    D_CS_CHECK(is_concurrent_container_v<DStCowState>);
    D_CS_CHECK(is_concurrent_container_v<st_atomic_value>);
    D_CS_CHECK(!is_concurrent_container_v<DStPlain>);
    return true;
}

// tests_aggregate_sequential
//   Verifies is_unsynchronized_container is the exact complement of
//   is_concurrent_container.
//   Tests the following:
//   - is_unsynchronized_container_v<T> == !is_concurrent_container_v<T> across
//     inputs
//   - the empty mutable type is sequential; strategy-bearing types are not
bool tests_aggregate_sequential()
{
    D_CS_CHECK(is_unsynchronized_container_v<DStPlain> ==
               !is_concurrent_container_v<DStPlain>);
    D_CS_CHECK(is_unsynchronized_container_v<DStLockedRw> ==
               !is_concurrent_container_v<DStLockedRw>);
    D_CS_CHECK(is_unsynchronized_container_v<DImmutableProbe> ==
               !is_concurrent_container_v<DImmutableProbe>);

    D_CS_CHECK(is_unsynchronized_container_v<DStPlain>);
    D_CS_CHECK(!is_unsynchronized_container_v<DStCowState>);
    D_CS_CHECK(!is_unsynchronized_container_v<DStHazard>);
    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
