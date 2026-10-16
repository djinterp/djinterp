/******************************************************************************
* djinterp [test]          concurrency_strategy_traits_tests_aggregate.cpp
*
*   Section IV.b - the vacuous / concurrent / synchronized / sequential axis.
* These are tested as the header's own logical invariants so they hold whatever
* the real is_immutable_container decides, while still pinning the concrete
* true/false values that do not depend on immutability.  Defines the
* "aggregate" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_aggregate_synchronized
//   Verifies is_synchronized_container is exactly the disjunction of the five
//   strategy predicates - the "runs machinery" question.
//   Tests the following:
//   - true for each strategy-bearing fixture, false for the empty type
//   - equals (locked || cow || rcu || atomic || hazard) on mixed inputs
bool
tests_aggregate_synchronized()
{
    D_CS_CHECK(is_synchronized_container_v<st_locked_rw>);
    D_CS_CHECK(is_synchronized_container_v<st_cow_state>);
    D_CS_CHECK(is_synchronized_container_v<st_rcu_protected>);
    D_CS_CHECK(is_synchronized_container_v<st_atomic_value>);
    D_CS_CHECK(is_synchronized_container_v<st_hazard>);
    D_CS_CHECK(is_synchronized_container_v<hy_locked_cow>);
    D_CS_CHECK(!is_synchronized_container_v<st_plain>);
    D_CS_CHECK(!is_synchronized_container_v<immutable_probe>);

    // the exact composition, on a hybrid input
    D_CS_CHECK(is_synchronized_container_v<hy_locked_cow> ==
               ( is_locked_container_v<hy_locked_cow> ||
                 is_cow_container_v<hy_locked_cow>    ||
                 is_rcu_container_v<hy_locked_cow>    ||
                 is_atomic_container_v<hy_locked_cow> ||
                 is_hazard_container_v<hy_locked_cow> ));
    return true;
}

// tests_aggregate_vacuous
//   Verifies is_vacuously_concurrent forwards is_immutable_container.  The
//   forwarding invariant holds regardless of the immutability model; the
//   immutable_probe additionally lights up the true arm under the reference
//   container_traits.
//   Tests the following:
//   - is_vacuously_concurrent_v<T> == is_immutable_container_v<T>
//   - a synchronized-but-mutable fixture is not vacuously concurrent
bool
tests_aggregate_vacuous()
{
    D_CS_CHECK(is_vacuously_concurrent_v<immutable_probe> ==
               is_immutable_container_v<immutable_probe>);
    D_CS_CHECK(is_vacuously_concurrent_v<st_plain> ==
               is_immutable_container_v<st_plain>);
    D_CS_CHECK(is_vacuously_concurrent_v<st_cow_state> ==
               is_immutable_container_v<st_cow_state>);

    // a live-machinery fixture is not immutable, so not vacuously concurrent
    D_CS_CHECK(!is_vacuously_concurrent_v<st_locked_rw>);
    D_CS_CHECK(is_vacuously_concurrent_v<immutable_probe>);
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
bool
tests_aggregate_concurrent()
{
    D_CS_CHECK(is_concurrent_container_v<st_locked_rw> ==
               ( is_synchronized_container_v<st_locked_rw> ||
                 is_vacuously_concurrent_v<st_locked_rw> ));
    D_CS_CHECK(is_concurrent_container_v<immutable_probe> ==
               ( is_synchronized_container_v<immutable_probe> ||
                 is_vacuously_concurrent_v<immutable_probe> ));
    D_CS_CHECK(is_concurrent_container_v<st_plain> ==
               ( is_synchronized_container_v<st_plain> ||
                 is_vacuously_concurrent_v<st_plain> ));

    D_CS_CHECK(is_concurrent_container_v<st_cow_state>);
    D_CS_CHECK(is_concurrent_container_v<st_atomic_value>);
    D_CS_CHECK(!is_concurrent_container_v<st_plain>);
    return true;
}

// tests_aggregate_sequential
//   Verifies is_sequential_container is the exact complement of
//   is_concurrent_container.
//   Tests the following:
//   - is_sequential_container_v<T> == !is_concurrent_container_v<T> across
//     inputs
//   - the empty mutable type is sequential; strategy-bearing types are not
bool
tests_aggregate_sequential()
{
    D_CS_CHECK(is_sequential_container_v<st_plain> ==
               !is_concurrent_container_v<st_plain>);
    D_CS_CHECK(is_sequential_container_v<st_locked_rw> ==
               !is_concurrent_container_v<st_locked_rw>);
    D_CS_CHECK(is_sequential_container_v<immutable_probe> ==
               !is_concurrent_container_v<immutable_probe>);

    D_CS_CHECK(is_sequential_container_v<st_plain>);
    D_CS_CHECK(!is_sequential_container_v<st_cow_state>);
    D_CS_CHECK(!is_sequential_container_v<st_hazard>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
