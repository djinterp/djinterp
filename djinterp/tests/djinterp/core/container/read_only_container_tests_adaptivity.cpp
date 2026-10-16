/*******************************************************************************
* djinterp [core]                       read_only_container_tests_adaptivity.cpp
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests_adaptivity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_only_observers_mirror_the_container
  the SFINAE guard, stated as an invariant.
  Tests the following:
  - the handle's observing surface matches the held container's, member for
    member, so wrapping neither invents nor loses an observer
  - the invariant holds across contiguous, node and fixed-size containers
  - it is the OBSERVERS that mirror; the mutators are absent by design
*/
bool
tests_read_only_observers_mirror_the_container()
{
    // vector: every observer present on both sides
    D_RO_CHECK(ro_observers_match(
                   ro_probe<dj::read_only_container<std::vector<int> > >(),
                   ro_probe<std::vector<int> >()));

    // array
    D_RO_CHECK(ro_observers_match(
                   ro_probe<dj::read_only_container<std::array<int, 4> > >(),
                   ro_probe<std::array<int, 4> >()));

    // list: no subscript, no data on either side
    D_RO_CHECK(ro_observers_match(
                   ro_probe<dj::read_only_container<std::list<int> > >(),
                   ro_probe<std::list<int> >()));

    // deque: subscript but no data on either side
    D_RO_CHECK(ro_observers_match(
                   ro_probe<dj::read_only_container<std::deque<int> > >(),
                   ro_probe<std::deque<int> >()));

    // string
    D_RO_CHECK(ro_observers_match(
                   ro_probe<dj::read_only_container<std::string> >(),
                   ro_probe<std::string>()));

    // the mutating side, by contrast, does NOT mirror
    D_RO_CHECK(ro_any_mutator(ro_probe<std::vector<int> >()));
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::vector<int> > >()));

    return true;
}

/*
tests_read_only_omits_subscript_for_list
  a member that must NOT materialise.
  Tests the following:
  - wrapping a container without random access does not conjure a subscript
    operator, because the forwarder's return type names the very call it
    forwards
  - the same handle still offers the observers the container does have
  - a container that DOES offer subscript keeps it, so the omission is
    specific rather than blanket
*/
bool
tests_read_only_omits_subscript_for_list()
{
    typedef dj::read_only_container<std::list<int> > list_handle;

    // std::list has no subscript, so neither does the handle
    D_RO_CHECK(!ro_has_sub<std::list<int> >::value);
    D_RO_CHECK(!ro_has_sub<list_handle>::value);

    // but the observers it does have came through
    D_RO_CHECK(ro_has_size<list_handle>::value);
    D_RO_CHECK(ro_has_empty<list_handle>::value);
    D_RO_CHECK(ro_has_front<list_handle>::value);
    D_RO_CHECK(ro_has_back<list_handle>::value);
    D_RO_CHECK(ro_has_begin<list_handle>::value);
    D_RO_CHECK(ro_has_end<list_handle>::value);
    D_RO_CHECK(ro_has_cbegin<list_handle>::value);
    D_RO_CHECK(ro_has_cend<list_handle>::value);

    // and a random-access container keeps its subscript
    D_RO_CHECK(ro_has_sub<
                   dj::read_only_container<std::vector<int> > >::value);
    D_RO_CHECK(ro_has_sub<
                   dj::read_only_container<std::deque<int> > >::value);
    D_RO_CHECK(ro_has_sub<
                   dj::read_only_container<std::array<int, 2> > >::value);

    return true;
}

/*
tests_read_only_omits_data_for_node_containers
  the storage accessor's availability.
  Tests the following:
  - data is absent for containers with no contiguous storage to address
  - it is present for the contiguous ones
  - the handle agrees with its held container in every case
*/
bool
tests_read_only_omits_data_for_node_containers()
{
    // a list has no contiguous storage
    D_RO_CHECK(!ro_has_data<std::list<int> >::value);
    D_RO_CHECK(!ro_has_data<dj::read_only_container<std::list<int> > >::value);

    // nor does a deque, despite offering random access
    D_RO_CHECK(!ro_has_data<std::deque<int> >::value);
    D_RO_CHECK(!ro_has_data<
                   dj::read_only_container<std::deque<int> > >::value);

    // a vector, array and string all do
    D_RO_CHECK(ro_has_data<std::vector<int> >::value);
    D_RO_CHECK(ro_has_data<
                   dj::read_only_container<std::vector<int> > >::value);
    D_RO_CHECK(ro_has_data<
                   dj::read_only_container<std::array<int, 3> > >::value);
    D_RO_CHECK(ro_has_data<dj::read_only_container<std::string> >::value);

    // the deque case is the interesting one: subscript without data
    typedef dj::read_only_container<std::deque<int> > deque_handle;

    D_RO_CHECK(ro_has_sub<deque_handle>::value);
    D_RO_CHECK(!ro_has_data<deque_handle>::value);

    return true;
}

/*
tests_read_only_supports_array_and_string
  the containers a sequence wrapper is most likely to mishandle.
  Tests the following:
  - a fixed-size container wraps and observes correctly, including its
    non-zero default size
  - a character container wraps and observes correctly
  - both offer the full contiguous observation surface
*/
bool
tests_read_only_supports_array_and_string()
{
    // a fixed-size container is never empty and knows its own size
    std::array<int, 3> raw = { { 7, 8, 9 } };

    const dj::read_only_container<std::array<int, 3> > a(raw);

    D_RO_CHECK(a.size() == 3u);
    D_RO_CHECK(!a.empty());
    D_RO_CHECK(a[0] == 7);
    D_RO_CHECK(a[2] == 9);
    D_RO_CHECK(a.front() == 7);
    D_RO_CHECK(a.back() == 9);
    D_RO_CHECK(a.data()[1] == 8);

    // a default-constructed one still reports its fixed size
    const dj::read_only_container<std::array<int, 3> > d;

    D_RO_CHECK(d.size() == 3u);
    D_RO_CHECK(!d.empty());

    // a character container
    const dj::read_only_container<std::string> s(std::string("hello"));

    D_RO_CHECK(s.size() == 5u);
    D_RO_CHECK(!s.empty());
    D_RO_CHECK(s[0] == 'h');
    D_RO_CHECK(s[4] == 'o');
    D_RO_CHECK(s.front() == 'h');
    D_RO_CHECK(s.back() == 'o');
    D_RO_CHECK(s.data()[1] == 'e');

    // both carry the full contiguous surface and no mutator
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::array<int, 3> > >()));
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::string> >()));

    return true;
}

/*
tests_read_only_is_usable_in_constant_expressions
  the constexpr forwarders.
  Tests the following:
  - a handle over a literal-type container can be built at compile time and
    observed there, so the D_CONSTEXPR on the forwarders is load-bearing
  - size, empty, subscript and front all answer in a constant expression
  - the compile-time answers agree with the run-time ones
*/
bool
tests_read_only_is_usable_in_constant_expressions()
{
    typedef dj::read_only_container<std::array<int, 3> > handle;

    // built and observed entirely at compile time
    constexpr std::array<int, 3> raw = { { 4, 5, 6 } };
    constexpr handle             h(raw);

    static_assert(h.size() == 3u, "size in a constant expression");
    static_assert(!h.empty(),     "empty in a constant expression");
    static_assert(h[0] == 4,      "subscript in a constant expression");
    static_assert(h[2] == 6,      "subscript in a constant expression");
    static_assert(h.front() == 4, "front in a constant expression");
    static_assert(h.back() == 6,  "back in a constant expression");

    // the same values at run time
    D_RO_CHECK(h.size() == 3u);
    D_RO_CHECK(h[0] == 4);
    D_RO_CHECK(h[2] == 6);
    D_RO_CHECK(h.front() == 4);
    D_RO_CHECK(h.back() == 6);

    // a default-constructed handle is a constant expression too
    constexpr handle d;

    static_assert(d.size() == 3u, "default ctor is constexpr");
    D_RO_CHECK(d.size() == 3u);

    return true;
}

/*
tests_read_only_detection_battery_is_sound
  the suite's own instrument.
  Tests the following:
  - the detectors report TRUE for members that genuinely exist on raw
    containers, so a false negative cannot silently mask a leaked mutator
  - they report FALSE for members those containers genuinely lack
  - the two roll-up helpers agree with the individual detectors
*/
bool
tests_read_only_detection_battery_is_sound()
{
    // a vector really has these
    D_RO_CHECK(ro_has_push_back<std::vector<int> >::value);
    D_RO_CHECK(ro_has_emplace_back<std::vector<int> >::value);
    D_RO_CHECK(ro_has_pop_back<std::vector<int> >::value);
    D_RO_CHECK(ro_has_clear<std::vector<int> >::value);
    D_RO_CHECK(ro_has_resize<std::vector<int> >::value);
    D_RO_CHECK(ro_has_reserve<std::vector<int> >::value);
    D_RO_CHECK(ro_has_insert<std::vector<int> >::value);
    D_RO_CHECK(ro_has_erase<std::vector<int> >::value);

    // and really lacks these
    D_RO_CHECK(!ro_has_push_front<std::vector<int> >::value);
    D_RO_CHECK(!ro_has_pop_front<std::vector<int> >::value);

    // a list is the mirror image on those two
    D_RO_CHECK(ro_has_push_front<std::list<int> >::value);
    D_RO_CHECK(ro_has_pop_front<std::list<int> >::value);
    D_RO_CHECK(!ro_has_reserve<std::list<int> >::value);

    // an array has essentially no mutating surface
    D_RO_CHECK(!ro_any_mutator(ro_probe<std::array<int, 3> >()));
    D_RO_CHECK(!ro_has_push_back<std::array<int, 3> >::value);
    D_RO_CHECK(!ro_has_clear<std::array<int, 3> >::value);

    // yet it has the full observation surface
    const ro_caps arr = ro_probe<std::array<int, 3> >();

    D_RO_CHECK(arr.size);
    D_RO_CHECK(arr.subscript);
    D_RO_CHECK(arr.data);

    // the roll-ups agree with the individual detectors
    const ro_caps vec = ro_probe<std::vector<int> >();

    D_RO_CHECK(ro_any_mutator(vec) ==
               (vec.push_back || vec.push_front || vec.emplace_back ||
                vec.pop_back  || vec.pop_front  || vec.clear        ||
                vec.resize    || vec.reserve    || vec.insert       ||
                vec.erase));
    D_RO_CHECK(ro_observers_match(vec, vec));
    D_RO_CHECK(!ro_observers_match(vec, ro_probe<std::list<int> >()));

    return true;
}

NS_END  // testing
NS_END  // djinterp
