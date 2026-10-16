/*******************************************************************************
* djinterp [core]                    bounded_container_traits_tests_snapshot.cpp
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests_snapshot.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cstring>
#include <list>
#include <type_traits>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_class_signals
  bounded_container_class - the six SIGNAL members.
  Tests the following:
  - has_extent, has_tuple_size, has_static_bounds, has_capacity, has_reserve
    and has_max_size each mirror their standalone signal trait
  - the all-ones row: every_signal fires all six at once
  - the all-zeros row: no_signal fires none

  NOTE the snapshot's membership: it reports six signals, and has_size_signal is
  NOT among them - even though has_size_signal is what drives is_unbounded and
  the `unknown` arm of capacity_bound_of.  A caller reading the snapshot
  therefore cannot see WHY a type came out `unknown` (no bound?  not a
  container?).  Adding `has_size` would close that, and this is the check that
  would then grow a line.
*/
bool
tests_class_signals()
{
    bool ok = true;

    using every = bounded_container_class<every_signal>;
    using none  = bounded_container_class<no_signal>;
    using vec   = bounded_container_class<std::vector<int>>;
    using arr   = bounded_container_class<std::array<int, 3>>;

    // all six signals, all true
    D_BCT_CHECK(every::has_extent);
    D_BCT_CHECK(every::has_tuple_size);
    D_BCT_CHECK(every::has_static_bounds);
    D_BCT_CHECK(every::has_capacity);
    D_BCT_CHECK(every::has_reserve);
    D_BCT_CHECK(every::has_max_size);

    // all six signals, all false
    D_BCT_CHECK(!none::has_extent);
    D_BCT_CHECK(!none::has_tuple_size);
    D_BCT_CHECK(!none::has_static_bounds);
    D_BCT_CHECK(!none::has_capacity);
    D_BCT_CHECK(!none::has_reserve);
    D_BCT_CHECK(!none::has_max_size);

    // a real growable container: capacity + reserve + max_size, nothing else
    D_BCT_CHECK(!vec::has_extent);
    D_BCT_CHECK(!vec::has_tuple_size);
    D_BCT_CHECK(!vec::has_static_bounds);
    D_BCT_CHECK(vec::has_capacity);
    D_BCT_CHECK(vec::has_reserve);
    D_BCT_CHECK(vec::has_max_size);

    // a real fixed container: tuple_size + max_size, nothing else
    D_BCT_CHECK(!arr::has_extent);
    D_BCT_CHECK(arr::has_tuple_size);
    D_BCT_CHECK(!arr::has_static_bounds);
    D_BCT_CHECK(!arr::has_capacity);
    D_BCT_CHECK(!arr::has_reserve);
    D_BCT_CHECK(arr::has_max_size);

    return ok;
}

/*
tests_class_verdicts
  bounded_container_class - the three VERDICT members.
  Tests the following:
  - is_bounded, is_unbounded and is_domain_bounded mirror their standalone
    traits on every fixture the suite defines
  - the verdicts stay mutually exclusive inside the snapshot too
*/
bool
tests_class_verdicts()
{
    bool ok = true;

    using arr      = bounded_container_class<std::array<int, 3>>;
    using vec      = bounded_container_class<std::vector<int>>;
    using interval = bounded_container_class<interval_container>;
    using none     = bounded_container_class<no_signal>;

    D_BCT_CHECK(arr::is_bounded);
    D_BCT_CHECK(!arr::is_unbounded);
    D_BCT_CHECK(!arr::is_domain_bounded);

    D_BCT_CHECK(!vec::is_bounded);
    D_BCT_CHECK(vec::is_unbounded);
    D_BCT_CHECK(!vec::is_domain_bounded);

    D_BCT_CHECK(interval::is_bounded);
    D_BCT_CHECK(!interval::is_unbounded);
    D_BCT_CHECK(interval::is_domain_bounded);

    D_BCT_CHECK(!none::is_bounded);
    D_BCT_CHECK(!none::is_unbounded);
    D_BCT_CHECK(!none::is_domain_bounded);

    // exclusivity, restated inside the snapshot
    D_BCT_CHECK(!(arr::is_bounded && arr::is_unbounded));
    D_BCT_CHECK(!(vec::is_bounded && vec::is_unbounded));
    D_BCT_CHECK(!(interval::is_bounded && interval::is_unbounded));

    return ok;
}

/*
tests_class_summary
  bounded_container_class - the two SUMMARY members.
  Tests the following:
  - `capacity` mirrors capacity_bound_of
  - `capacity_name` is capacity_bound_name applied to that same value, so the
    two can never drift
  - all three enum states appear
*/
bool
tests_class_summary()
{
    bool ok = true;

    using arr  = bounded_container_class<std::array<int, 3>>;
    using vec  = bounded_container_class<std::vector<int>>;
    using none = bounded_container_class<no_signal>;

    D_BCT_CHECK(arr::capacity == capacity_bound::bounded);
    D_BCT_CHECK(vec::capacity == capacity_bound::unbounded);
    D_BCT_CHECK(none::capacity == capacity_bound::unknown);

    D_BCT_CHECK(std::strcmp(arr::capacity_name, "bounded") == 0);
    D_BCT_CHECK(std::strcmp(vec::capacity_name, "unbounded") == 0);
    D_BCT_CHECK(std::strcmp(none::capacity_name, "unknown") == 0);

    // the name is derived from the value, so they agree by construction - this
    // is the check that keeps it that way
    D_BCT_CHECK(std::strcmp(arr::capacity_name,
                            capacity_bound_name(arr::capacity)) == 0);
    D_BCT_CHECK(std::strcmp(vec::capacity_name,
                            capacity_bound_name(vec::capacity)) == 0);
    D_BCT_CHECK(std::strcmp(none::capacity_name,
                            capacity_bound_name(none::capacity)) == 0);

    return ok;
}

/*
tests_class_agrees_with_traits
  The snapshot must be a faithful re-statement of the standalone traits - it is
  a convenience, and a convenience that can disagree with its sources is worse
  than no convenience at all.
  Tests the following:
  - all eleven members agree with their sources, on every fixture in the suite
  - including the cv-ref forms, since the snapshot cleans its argument too
*/
bool
tests_class_agrees_with_traits()
{
    bool ok = true;

    #define D_BCT_SNAPSHOT_AGREES(...)                                        \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_extent ==              \
                    has_fixed_extent_signal<__VA_ARGS__>::value);                    \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_tuple_size ==          \
                    has_tuple_size_signal<__VA_ARGS__>::value);                      \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_static_bounds ==       \
                    has_static_bounds_signal<__VA_ARGS__>::value);                   \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_capacity ==            \
                    has_capacity_signal<__VA_ARGS__>::value);                        \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_reserve ==             \
                    has_reserve_signal<__VA_ARGS__>::value);                         \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::has_max_size ==            \
                    has_max_size_signal<__VA_ARGS__>::value);                        \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::is_bounded ==              \
                    is_bounded_container<__VA_ARGS__>::value);                       \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::is_unbounded ==            \
                    is_unbounded_container<__VA_ARGS__>::value);                     \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::is_domain_bounded ==       \
                    is_domain_bounded_container<__VA_ARGS__>::value);                \
        D_BCT_CHECK(bounded_container_class<__VA_ARGS__>::capacity ==                \
                    capacity_bound_of<__VA_ARGS__>::value);                          \
        D_BCT_CHECK(std::strcmp(                                              \
                        bounded_container_class<__VA_ARGS__>::capacity_name,         \
                        capacity_bound_name(                                  \
                            capacity_bound_of<__VA_ARGS__>::value)) == 0)

    D_BCT_SNAPSHOT_AGREES(std::array<int, 3>);
    D_BCT_SNAPSHOT_AGREES(std::vector<int>);
    D_BCT_SNAPSHOT_AGREES(std::list<int>);
    D_BCT_SNAPSHOT_AGREES(interval_container);
    D_BCT_SNAPSHOT_AGREES(fixed_capacity);
    D_BCT_SNAPSHOT_AGREES(growable);
    D_BCT_SNAPSHOT_AGREES(growable_nullary_reserve);
    D_BCT_SNAPSHOT_AGREES(extent_static_member);
    D_BCT_SNAPSHOT_AGREES(extent_and_growable);
    D_BCT_SNAPSHOT_AGREES(bounds_both);
    D_BCT_SNAPSHOT_AGREES(tuple_size_conformer);
    D_BCT_SNAPSHOT_AGREES(size_const);
    D_BCT_SNAPSHOT_AGREES(max_size_only);
    D_BCT_SNAPSHOT_AGREES(every_signal);
    D_BCT_SNAPSHOT_AGREES(no_signal);
    D_BCT_SNAPSHOT_AGREES(int);

    // it cleans its argument, so the cv-ref forms agree with the bare one
    D_BCT_SNAPSHOT_AGREES(const std::vector<int>&);
    D_BCT_SNAPSHOT_AGREES(std::array<int, 3>&&);

    #undef D_BCT_SNAPSHOT_AGREES

    return ok;
}

/*
tests_class_is_constexpr
  Every member of the snapshot is a `static constexpr`, so the whole thing is
  usable at compile time.
  Tests the following:
  - the bools are constant expressions (each is used as a template argument,
    which is the only way to prove it)
  - the enum member is a constant expression
  - the const char* member is a constant expression
  - the snapshot is an empty type: it carries no storage, only compile-time
    answers, so instantiating one costs nothing
*/
bool
tests_class_is_constexpr()
{
    bool ok = true;

    using arr = bounded_container_class<std::array<int, 3>>;
    using vec = bounded_container_class<std::vector<int>>;

    // a bool constant expression: usable as a template argument
    D_BCT_CHECK((std::integral_constant<bool, arr::is_bounded>::value));
    D_BCT_CHECK((std::integral_constant<bool, arr::has_tuple_size>::value));
    D_BCT_CHECK((std::integral_constant<bool, vec::is_unbounded>::value));
    D_BCT_CHECK((std::integral_constant<bool, vec::has_reserve>::value));

    // an enum constant expression
    D_BCT_CHECK((std::integral_constant<capacity_bound,
                                        arr::capacity>::value ==
                 capacity_bound::bounded));

    // a const char* constant expression
    D_CONSTEXPR const char* name = arr::capacity_name;

    D_BCT_CHECK(std::strcmp(name, "bounded") == 0);

    // and it is a pure compile-time carrier
    D_BCT_CHECK(std::is_empty<arr>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
