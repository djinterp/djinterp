/*******************************************************************************
* djinterp [core]                      bounded_container_traits_tests_stdlib.cpp
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests_stdlib.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <bitset>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_stdlib_array
  std::array - the bounded case the design is aimed at.
  Tests the following:
  - promoted by the tuple_size signal, and by that signal alone: it has no
    `extent` member, no capacity() and no reserve()
  - the zero-length array is bounded too
  - it exposes max_size(), which is exactly why max_size cannot be a promoting
    signal - std::vector exposes it as well
*/
bool
tests_stdlib_array()
{
    bool ok = true;

    D_BCT_CHECK(is_bounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(!is_unbounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(capacity_bound_of<std::array<int, 3>>::value ==
                capacity_bound::bounded);

    // exactly one signal carries it
    D_BCT_CHECK(has_tuple_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!has_static_bounds_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!has_reserve_signal<std::array<int, 3>>::value);

    // container-shaped, and domain-free
    D_BCT_CHECK(has_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(has_max_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!is_domain_bounded_container<std::array<int, 3>>::value);

    // the zero-length array
    D_BCT_CHECK(is_bounded_container<std::array<int, 0>>::value);
    D_BCT_CHECK(capacity_bound_of<std::array<int, 0>>::value ==
                capacity_bound::bounded);

    return ok;
}

/*
tests_stdlib_vector
  std::vector - the unbounded case the design is aimed at.
  Tests the following:
  - it has a capacity(), so the fourth disjunct is live; it also has a reserve(),
    which is what kills it.  Remove the anti-signal and std::vector becomes
    "bounded", which is the whole reason the anti-signal exists
  - no other signal fires
*/
bool
tests_stdlib_vector()
{
    bool ok = true;

    D_BCT_CHECK(!is_bounded_container<std::vector<int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::vector<int>>::value);
    D_BCT_CHECK(capacity_bound_of<std::vector<int>>::value ==
                capacity_bound::unbounded);

    // the fourth disjunct is live, and the anti-signal is what closes it
    D_BCT_CHECK(has_capacity_signal<std::vector<int>>::value);
    D_BCT_CHECK(has_reserve_signal<std::vector<int>>::value);

    D_BCT_CHECK(!has_fixed_extent_signal<std::vector<int>>::value);
    D_BCT_CHECK(!has_tuple_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(!has_static_bounds_signal<std::vector<int>>::value);

    D_BCT_CHECK(has_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(has_max_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(!is_domain_bounded_container<std::vector<int>>::value);

    // std::vector<bool> is a different type, and must classify the same way
    D_BCT_CHECK(is_unbounded_container<std::vector<bool>>::value);

    return ok;
}

/*
tests_stdlib_node_containers
  The node-based sequences: no capacity(), no reserve(), no extent.
  Tests the following:
  - std::list and std::deque are unbounded on the strength of size() alone
  - std::forward_list is UNKNOWN, not unbounded

  THE FINDING.  std::forward_list has no size() member - it never has, by design
  (LWG's decision: an O(1) size would cost a word per list).  The container guard
  in is_unbounded_container is has_size_signal, so the guard never opens and a
  genuine, growable, standard sequence container classifies as `unknown` - the
  state reserved for "not a container".  Anything that dispatches on
  capacity_bound will silently take the not-a-container branch for it.

  Widening the guard - to `size() || begin()/end()`, say, or to an iterable
  probe - would fix it.  Pinned here as it stands.
*/
bool
tests_stdlib_node_containers()
{
    bool ok = true;

    // list and deque: unbounded, as expected
    D_BCT_CHECK(is_unbounded_container<std::list<int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::deque<int>>::value);
    D_BCT_CHECK(capacity_bound_of<std::list<int>>::value ==
                capacity_bound::unbounded);
    D_BCT_CHECK(capacity_bound_of<std::deque<int>>::value ==
                capacity_bound::unbounded);

    // neither has a capacity() - std::deque grows without one
    D_BCT_CHECK(!has_capacity_signal<std::list<int>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::deque<int>>::value);
    D_BCT_CHECK(!has_reserve_signal<std::list<int>>::value);
    D_BCT_CHECK(!has_reserve_signal<std::deque<int>>::value);

    // forward_list: no size(), so the container guard never opens
    D_BCT_CHECK(!has_size_signal<std::forward_list<int>>::value);
    D_BCT_CHECK(!is_bounded_container<std::forward_list<int>>::value);
    D_BCT_CHECK(!is_unbounded_container<std::forward_list<int>>::value);
    D_BCT_CHECK(capacity_bound_of<std::forward_list<int>>::value ==
                capacity_bound::unknown);

    return ok;
}

/*
tests_stdlib_string
  std::string and std::string_view.
  Tests the following:
  - std::string is shaped exactly like std::vector - capacity + reserve + size -
    and classifies the same way
  - std::string_view has a size() and nothing else, so it is unbounded.  A view
    over a fixed extent is arguably bounded, but it advertises nothing that says
    so, and the trait can only read what is advertised
*/
bool
tests_stdlib_string()
{
    bool ok = true;

    D_BCT_CHECK(is_unbounded_container<std::string>::value);
    D_BCT_CHECK(has_capacity_signal<std::string>::value);
    D_BCT_CHECK(has_reserve_signal<std::string>::value);
    D_BCT_CHECK(capacity_bound_of<std::string>::value ==
                capacity_bound::unbounded);

    D_BCT_CHECK(is_unbounded_container<std::string_view>::value);
    D_BCT_CHECK(!has_capacity_signal<std::string_view>::value);
    D_BCT_CHECK(!has_reserve_signal<std::string_view>::value);
    D_BCT_CHECK(has_size_signal<std::string_view>::value);

    return ok;
}

/*
tests_stdlib_tuple_pair
  std::tuple and std::pair - bounded, and not containers.
  Tests the following:
  - both are promoted by the tuple_size signal
  - NEITHER has a size(), so neither is a container by the header's guard - and
    is_bounded_container is deliberately not gated on size(), so both come out
    `bounded` rather than `unknown`
  - the empty tuple is bounded too (kappa = 0)
  - std::bitset, by contrast, has a size() and no promoting signal at all - so
    the archetypal fixed-width container classifies as UNBOUNDED

  THE FINDING (bitset).  std::bitset<8> presents a constexpr size() returning 8,
  and nothing else the header looks for: no `extent`, no std::tuple_size
  specialization, no capacity().  It is therefore "a container with no bounding
  evidence", i.e. kappa = inf.  A fixed-width bitset is as bounded as a type can
  be.  Either bitset needs the djinterp `extent` convention applied to it, or the
  extent probe needs to reach a constexpr size().  Pinned as it stands.
*/
bool
tests_stdlib_tuple_pair()
{
    bool ok = true;

    // tuple / pair: bounded, but not containers
    D_BCT_CHECK(is_bounded_container<std::tuple<int, char, long>>::value);
    D_BCT_CHECK(is_bounded_container<std::tuple<>>::value);
    D_BCT_CHECK(is_bounded_container<std::pair<int, char>>::value);

    D_BCT_CHECK(!has_size_signal<std::tuple<int, char>>::value);
    D_BCT_CHECK(!has_size_signal<std::pair<int, char>>::value);

    D_BCT_CHECK(!is_unbounded_container<std::tuple<int, char>>::value);
    D_BCT_CHECK(capacity_bound_of<std::tuple<int, char>>::value ==
                capacity_bound::bounded);
    D_BCT_CHECK(capacity_bound_of<std::pair<int, char>>::value ==
                capacity_bound::bounded);

    // bitset: a container with a fixed width, classified unbounded
    D_BCT_CHECK(has_size_signal<std::bitset<8>>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<std::bitset<8>>::value);
    D_BCT_CHECK(!has_tuple_size_signal<std::bitset<8>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::bitset<8>>::value);
    D_BCT_CHECK(!is_bounded_container<std::bitset<8>>::value);
    D_BCT_CHECK(is_unbounded_container<std::bitset<8>>::value);
    D_BCT_CHECK(capacity_bound_of<std::bitset<8>>::value ==
                capacity_bound::unbounded);

    // initializer_list and valarray: size(), nothing else -> unbounded
    D_BCT_CHECK(is_unbounded_container<std::initializer_list<int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::valarray<int>>::value);

    return ok;
}

/*
tests_stdlib_associative
  The associative and unordered-associative containers.
  Tests the following:
  - the ordered ones (set, map) have size() and max_size() and no capacity, so
    they are unbounded
  - the UNORDERED ones have a reserve(size_type) but no capacity().  The fourth
    disjunct needs a capacity() to be about, so the lone anti-signal promotes
    nothing and they too come out unbounded - which is the right answer, reached
    by the right reasoning.  Had the disjunct been written `!reserve` rather than
    `capacity && !reserve`, every container without a reserve() would have been
    promoted; this is the check that says it was not
*/
bool
tests_stdlib_associative()
{
    bool ok = true;

    // ordered
    D_BCT_CHECK(is_unbounded_container<std::set<int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::map<int, int>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::set<int>>::value);
    D_BCT_CHECK(!has_reserve_signal<std::set<int>>::value);

    // unordered: reserve WITHOUT capacity
    D_BCT_CHECK(has_reserve_signal<std::unordered_map<int, int>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::unordered_map<int, int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::unordered_map<int, int>>::value);

    D_BCT_CHECK(has_reserve_signal<std::unordered_set<int>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::unordered_set<int>>::value);
    D_BCT_CHECK(is_unbounded_container<std::unordered_set<int>>::value);

    D_BCT_CHECK(capacity_bound_of<std::unordered_map<int, int>>::value ==
                capacity_bound::unbounded);

    // none of them is domain-bounded
    D_BCT_CHECK(!is_domain_bounded_container<std::set<int>>::value);
    D_BCT_CHECK(!is_domain_bounded_container<std::unordered_set<int>>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
