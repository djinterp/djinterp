/*******************************************************************************
* djinterp [core]                                 lock_policy_tests_selector.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_selector.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                   lock_policy_tests_selector.cpp
//   Section VII: select_lock_policy - mapping a level to a policy (C++11+).
//
//   The selector is the header's one piece of real metaprogramming: a template
// that maps each thread_safety_leavel to a concrete policy, via a table of
// specializations in the internal namespace, exposed through the
// select_lock_policy alias.  Two things make it worth careful testing.  First,
// two DISTINCT levels collapse to the same policy: none and atomic_only both
// map
// to null_lock_policy, because lock-free-atomic code needs no mutex.  Second,
// and most important, the selector FALLS BACK by standard - it always resolves
// to SOME policy, but which one depends on the build:
// select_lock_policy<shared>
// is shared_lock_policy at C++17 and exclusive_lock_policy below it, and
// select_lock_policy<shared_timed> is shared_timed_lock_policy from C++14 and
// timed_lock_policy below it.
//
//   So the two fallback tests check the mapping THAT HOLDS FOR THIS BUILD, not
// the C++17 answer.  The selector itself always yields a valid type, so
// select_lock_policy<L> can be named unconditionally; only the higher POLICY
// names (shared_lock_policy, shared_timed_lock_policy) are gated, since below
// their standards those names do not exist.

// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_selector_none_maps_to_null
  the bottom level selects the null policy.
  Tests the following:
  - select_lock_policy<none> is null_lock_policy
  So single-threaded code, classified at level none, gets the zero-cost policy.
*/
bool
tests_selector_none_maps_to_null()
{
    D_CONSTEXPR bool maps =
        std::is_same<select_lock_policy<thread_safety_leavel::none>,
                     null_lock_policy>::value;

    static_assert(maps, "select_lock_policy<none> must be null_lock_policy");

    return maps;
}

/*
tests_selector_atomic_only_also_maps_to_null
  the subtle one: atomic_only ALSO selects the null policy.
  Tests the following:
  - select_lock_policy<atomic_only> is null_lock_policy - the SAME policy that
    none maps to
  - so two distinct levels share one policy
  The reasoning is that atomic_only means "lock-free atomics, no mutex", so the
  right lock policy is the no-op one.  A caller who expects a distinct
  atomic-specific policy here would be wrong; the mapping deliberately reuses
  null.  Pinned so the collapse is on record.
*/
bool
tests_selector_atomic_only_also_maps_to_null()
{
    D_CONSTEXPR bool maps_to_null =
        std::is_same<select_lock_policy<thread_safety_leavel::atomic_only>,
                     null_lock_policy>::value;

    // and it is genuinely the same policy that `none` selects
    D_CONSTEXPR bool same_as_none =
        std::is_same<
            select_lock_policy<thread_safety_leavel::atomic_only>,
            select_lock_policy<thread_safety_leavel::none> >::value;

    static_assert(maps_to_null && same_as_none,
                  "atomic_only must select null_lock_policy, as none does");

    return (maps_to_null && same_as_none);
}

/*
tests_selector_exclusive_maps_to_exclusive
  the exclusive level selects the exclusive policy.
  Tests the following:
  - select_lock_policy<exclusive> is exclusive_lock_policy
*/
bool
tests_selector_exclusive_maps_to_exclusive()
{
    D_CONSTEXPR bool maps =
        std::is_same<select_lock_policy<thread_safety_leavel::exclusive>,
                     exclusive_lock_policy>::value;

    static_assert(maps,
                  "select<exclusive> must be exclusive_lock_policy");

    return maps;
}

/*
tests_selector_timed_maps_to_timed
  the timed level selects the timed policy.
  Tests the following:
  - select_lock_policy<timed> is timed_lock_policy
*/
bool
tests_selector_timed_maps_to_timed()
{
    D_CONSTEXPR bool maps =
        std::is_same<select_lock_policy<thread_safety_leavel::timed>,
                     timed_lock_policy>::value;

    static_assert(maps,
                  "select_lock_policy<timed> must be timed_lock_policy");

    return maps;
}

/*
tests_selector_shared_maps_correctly_for_this_standard
  the shared level maps to shared_lock_policy - or falls back to exclusive.
  Tests the following:
  - at C++17 and above, select_lock_policy<shared> is shared_lock_policy
  - below C++17, where shared_lock_policy does not exist, it FALLS BACK to
    exclusive_lock_policy
  - the suite asserts whichever mapping is correct for the build's standard
  This is the fallback's whole purpose: code can ask for `shared` on any
  standard
  and get a working policy - the best one available - rather than a compile
  error.  The higher policy name is only mentioned inside the C++17 guard,
  because below C++17 naming it would itself be an error.
*/
bool
tests_selector_shared_maps_correctly_for_this_standard()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool maps =
        std::is_same<select_lock_policy<thread_safety_leavel::shared>,
                     shared_lock_policy>::value;

    static_assert(maps,
                  "at C++17 select_lock_policy<shared> is shared_lock_policy");

    return maps;
#else
    D_CONSTEXPR bool falls_back =
        std::is_same<select_lock_policy<thread_safety_leavel::shared>,
                     exclusive_lock_policy>::value;

    static_assert(falls_back,
                  "below C++17 select_lock_policy<shared> falls back to "
                  "exclusive_lock_policy");

    return falls_back;
#endif
}

/*
tests_selector_shared_timed_maps_correctly_for_this_standard
  the shared_timed level maps to shared_timed_lock_policy - or falls back to
  timed.
  Tests the following:
  - at C++14 and above, select_lock_policy<shared_timed> is
    shared_timed_lock_policy
  - below C++14, where that policy does not exist, it FALLS BACK to
    timed_lock_policy
  - the suite asserts whichever mapping is correct for the build's standard
  Note the boundary is C++14, not C++17: std::shared_timed_mutex predates
  std::shared_mutex by a standard, so this fallback resolves to the real policy
  one standard earlier than the plain `shared` one does.
*/
bool
tests_selector_shared_timed_maps_correctly_for_this_standard()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool maps =
        std::is_same<select_lock_policy<thread_safety_leavel::shared_timed>,
                     shared_timed_lock_policy>::value;

    static_assert(maps,
                  "at C++14 select_lock_policy<shared_timed> is "
                  "shared_timed_lock_policy");

    return maps;
#else
    D_CONSTEXPR bool falls_back =
        std::is_same<select_lock_policy<thread_safety_leavel::shared_timed>,
                     timed_lock_policy>::value;

    static_assert(falls_back,
                  "below C++14 select_lock_policy<shared_timed> falls back to "
                  "timed_lock_policy");

    return falls_back;
#endif
}

/*
tests_select_lock_policy_is_the_public_alias_over_the_impl
  the public alias is a thin skin over the internal impl.
  Tests the following:
  - select_lock_policy<L> is exactly internal::select_lock_policy_impl<L>::type
  - checked at the exclusive level (defined on every supported standard)
  So the alias adds no logic of its own - it just reaches through to the
  specialization table, which is where all the per-standard fallback lives.
*/
bool
tests_select_lock_policy_is_the_public_alias_over_the_impl()
{
    D_CONSTEXPR bool same =
        std::is_same<
            select_lock_policy<thread_safety_leavel::exclusive>,
            internal::select_lock_policy_impl<
                thread_safety_leavel::exclusive>::type>::value;

    static_assert(same,
                  "select_lock_policy must alias the internal impl's type");

    return same;
}

/*
tests_selector_resolves_every_level
  the selector yields a valid policy for ALL six levels.
  Tests the following:
  - select_lock_policy<L> is a complete type for none, atomic_only, exclusive,
    timed, shared and shared_timed
  - so there is no level the primary (undefined) template leaks through: every
    level has a specialization, on every supported standard, thanks to the
    fallbacks
  This is the completeness check behind the mapping tests: not only are the
  specific mappings right, but no level is left unmapped.
*/
bool
tests_selector_resolves_every_level()
{
    const bool all_resolve =
        ( (sizeof(select_lock_policy<thread_safety_leavel::none>) > 0)        &&
          (sizeof(select_lock_policy<thread_safety_leavel::atomic_only>) > 0) &&
          (sizeof(select_lock_policy<thread_safety_leavel::exclusive>) > 0)   &&
          (sizeof(select_lock_policy<thread_safety_leavel::timed>) > 0)       &&
          (sizeof(select_lock_policy<thread_safety_leavel::shared>) > 0)      &&
          (sizeof(select_lock_policy<
                      thread_safety_leavel::shared_timed>) > 0) );

    return all_resolve;
}


NS_END  // testing
NS_END  // djinterp
