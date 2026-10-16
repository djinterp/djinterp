/*******************************************************************************
* djinterp [core]                        write_only_container_tests_metadata.cpp
*
*
* path:      /tests/djinterp/core/container/write_only_container_tests_metadata.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "write_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_write_only_size_reports_the_count
  the element count.
  Tests the following:
  - size reports the held container's element count
  - it follows the container the sink was built from, not a default
  - it is a const member, answering on a const sink
*/
bool
tests_write_only_size_reports_the_count()
{
    const dj::write_only_container<std::vector<int> > e;

    D_WO_CHECK(e.size() == 0u);

    const dj::write_only_container<std::vector<int> > three(
        std::size_t(3), 7);

    D_WO_CHECK(three.size() == 3u);

    const dj::write_only_container<std::vector<int> > one(std::size_t(1), 0);

    D_WO_CHECK(one.size() == 1u);

    // a node container, where size is not a pointer subtraction
    const dj::write_only_container<std::list<int> > l(std::size_t(5), 1);

    D_WO_CHECK(l.size() == 5u);

    // a fixed-size container reports its own size
    const dj::write_only_container<std::array<int, 4> > a;

    D_WO_CHECK(a.size() == 4u);

    // a character container
    const dj::write_only_container<std::string> s(std::string("hello"));

    D_WO_CHECK(s.size() == 5u);

    // the return type is the container's own size type
    D_WO_CHECK((std::is_same<decltype(e.size()),
                             std::vector<int>::size_type>::value));

    return true;
}

/*
tests_write_only_empty_agrees_with_size
  the emptiness predicate.
  Tests the following:
  - empty is true exactly when size is zero
  - the two agree across every container tried
  - both answer on a const sink
*/
bool
tests_write_only_empty_agrees_with_size()
{
    const dj::write_only_container<std::vector<int> > e;

    D_WO_CHECK(e.empty());
    D_WO_CHECK(e.empty() == (e.size() == 0u));

    const dj::write_only_container<std::vector<int> > full(
        std::size_t(2), 1);

    D_WO_CHECK(!full.empty());
    D_WO_CHECK(full.empty() == (full.size() == 0u));

    // a node container
    const dj::write_only_container<std::list<int> > le;
    const dj::write_only_container<std::list<int> > lf(std::size_t(1), 1);

    D_WO_CHECK(le.empty() == (le.size() == 0u));
    D_WO_CHECK(lf.empty() == (lf.size() == 0u));
    D_WO_CHECK(le.empty());
    D_WO_CHECK(!lf.empty());

    // a fixed-size container is never empty
    const dj::write_only_container<std::array<int, 2> > a;

    D_WO_CHECK(!a.empty());
    D_WO_CHECK(a.empty() == (a.size() == 0u));

    // the return type is a plain bool
    D_WO_CHECK((std::is_same<decltype(e.empty()), bool>::value));

    return true;
}

/*
tests_write_only_metadata_tracks_appends
  the observers following the sink's own operations.
  Tests the following:
  - size grows by exactly one per append
  - empty flips as soon as the first element arrives
  - clear returns both observers to their initial answers
  - the sink is not caching a stale count
*/
bool
tests_write_only_metadata_tracks_appends()
{
    dj::write_only_container<std::vector<int> > sink;

    D_WO_CHECK(sink.empty());
    D_WO_CHECK(sink.size() == 0u);

    sink.push_back(1);
    D_WO_CHECK(!sink.empty());
    D_WO_CHECK(sink.size() == 1u);

    sink.push_back(2);
    D_WO_CHECK(sink.size() == 2u);

    sink.emplace_back(3);
    D_WO_CHECK(sink.size() == 3u);

    // repeated appends keep counting
    std::size_t i = 0;

    for (i = 0; i < 10u; ++i)
    {
        sink.push_back(static_cast<int>(i));
    }

    D_WO_CHECK(sink.size() == 13u);
    D_WO_CHECK(!sink.empty());

    // clear returns to the initial state
    sink.clear();
    D_WO_CHECK(sink.empty());
    D_WO_CHECK(sink.size() == 0u);

    // and counting resumes correctly
    sink.push_back(9);
    D_WO_CHECK(sink.size() == 1u);

    // a node container behaves the same way
    dj::write_only_container<std::list<int> > l;

    l.push_back(1);
    l.push_back(2);
    D_WO_CHECK(l.size() == 2u);
    l.clear();
    D_WO_CHECK(l.empty());

    return true;
}

/*
tests_write_only_metadata_is_value_free
  why these two observers may survive the seal.
  Tests the following:
  - neither returns an element, nor a reference or pointer to one, so
    neither reveals what any stored value is
  - size returns a count and empty a bool
  - two sinks holding different values but the same COUNT are
    indistinguishable through this surface, which is the property that
    makes it safe
*/
bool
tests_write_only_metadata_is_value_free()
{
    typedef dj::write_only_container<std::vector<int> > sink;
    typedef sink::value_type                            element;

    dj::write_only_container<std::vector<int> > s;

    // neither observer's return type is the element type
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), const element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element*>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), const element*>::value));

    D_WO_CHECK(!(std::is_same<decltype(s.empty()), element>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.empty()), element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.empty()), const element&>::value));

    // what they DO return
    D_WO_CHECK((std::is_same<decltype(s.size()),
                             std::vector<int>::size_type>::value));
    D_WO_CHECK((std::is_same<decltype(s.empty()), bool>::value));
    D_WO_CHECK(std::is_integral<decltype(s.size())>::value);

    // two sinks with the same count but different contents cannot be told
    // apart through this surface
    dj::write_only_container<std::vector<int> > a;
    dj::write_only_container<std::vector<int> > b;

    a.push_back(1);
    a.push_back(2);
    b.push_back(100);
    b.push_back(200);

    D_WO_CHECK(a.size() == b.size());
    D_WO_CHECK(a.empty() == b.empty());

    // and a character sink is likewise value-free
    dj::write_only_container<std::string> t;

    D_WO_CHECK(!(std::is_same<decltype(t.size()), char>::value));
    D_WO_CHECK(!(std::is_same<decltype(t.size()), char&>::value));

    return true;
}

/*
tests_write_only_metadata_is_constexpr
  compile-time usability of the surviving observers.
  Tests the following:
  - size and empty answer in a constant expression when the held container
    permits it, so the D_CONSTEXPR on them is load-bearing
  - a default-constructed sink is itself a constant expression
  - the compile-time answers agree with the run-time ones
*/
bool
tests_write_only_metadata_is_constexpr()
{
    typedef dj::write_only_container<std::array<int, 3> > sink;

    constexpr std::array<int, 3> raw = { { 4, 5, 6 } };
    constexpr sink               s(raw);

    static_assert(s.size() == 3u, "size in a constant expression");
    static_assert(!s.empty(),     "empty in a constant expression");

    D_WO_CHECK(s.size() == 3u);
    D_WO_CHECK(!s.empty());

    // a default-constructed sink is a constant expression too
    constexpr sink d;

    static_assert(d.size() == 3u, "default ctor is constexpr");
    D_WO_CHECK(d.size() == 3u);

    // an empty fixed-size container really does report empty
    typedef dj::write_only_container<std::array<int, 0> > empty_sink;

    constexpr empty_sink e;

    static_assert(e.size() == 0u, "zero-length array reports zero");
    static_assert(e.empty(),      "zero-length array reports empty");
    D_WO_CHECK(e.empty());

    return true;
}

NS_END  // testing
NS_END  // djinterp
