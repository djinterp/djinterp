/*******************************************************************************
* djinterp [core]                        read_write_container_tests_mutation.cpp
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests_mutation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_write_push_back_appends
  the append operation.
  Tests the following:
  - push_back adds exactly one element, at the end
  - the reported size grows by one each time
  - the earlier elements keep their values and order
  - it works on every container that offers it
*/
bool
tests_read_write_push_back_appends()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(h.back() == 1);
    D_RW_CHECK(h.front() == 1);

    h.push_back(2);
    h.push_back(3);
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);
    D_RW_CHECK(h[2] == 3);
    D_RW_CHECK(h.back() == 3);
    D_RW_CHECK(h.front() == 1);

    // an lvalue argument is accepted as readily as a temporary
    int value = 4;

    h.push_back(value);
    D_RW_CHECK(h.size() == 4u);
    D_RW_CHECK(h.back() == 4);

    // a node container
    dj::read_write_container<std::list<int> > l;

    l.push_back(7);
    l.push_back(8);
    D_RW_CHECK(l.size() == 2u);
    D_RW_CHECK(l.front() == 7);
    D_RW_CHECK(l.back() == 8);

    // a character container
    dj::read_write_container<std::string> s;

    s.push_back('a');
    s.push_back('b');
    D_RW_CHECK(s.size() == 2u);
    D_RW_CHECK(s[0] == 'a');
    D_RW_CHECK(s[1] == 'b');

    return true;
}

/*
tests_read_write_push_front_prepends
  the front-insertion operation.
  Tests the following:
  - push_front adds exactly one element, at the front
  - the previous elements shift back, keeping their order
  - it is available for the containers that offer it and absent for those
    that do not
*/
bool
tests_read_write_push_front_prepends()
{
    dj::read_write_container<std::list<int> > l;

    l.push_front(1);
    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front() == 1);

    l.push_front(2);
    l.push_front(3);
    D_RW_CHECK(l.size() == 3u);
    D_RW_CHECK(l.front() == 3);
    D_RW_CHECK(l.back() == 1);

    // a deque offers it with random access, so the order is checkable
    dj::read_write_container<std::deque<int> > d;

    d.push_back(2);
    d.push_front(1);
    d.push_back(3);

    D_RW_CHECK(d.size() == 3u);
    D_RW_CHECK(d[0] == 1);
    D_RW_CHECK(d[1] == 2);
    D_RW_CHECK(d[2] == 3);

    // a vector does not offer it, so neither does its handle
    D_RW_CHECK(!rw_has_push_front<std::vector<int> >::value);
    D_RW_CHECK(!rw_has_push_front<
                   dj::read_write_container<std::vector<int> > >::value);

    // while the list and deque handles do
    D_RW_CHECK(rw_has_push_front<
                   dj::read_write_container<std::list<int> > >::value);
    D_RW_CHECK(rw_has_push_front<
                   dj::read_write_container<std::deque<int> > >::value);

    return true;
}

/*
tests_read_write_emplace_back_forwards_args
  in-place construction at the end.
  Tests the following:
  - emplace_back appends one element
  - several arguments reach the ELEMENT's constructor, not the container's
  - the constructed element carries every argument correctly
*/
bool
tests_read_write_emplace_back_forwards_args()
{
    dj::read_write_container<std::vector<int> > h;

    h.emplace_back(1);
    h.emplace_back(2);

    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);

    // several arguments, forwarded to a two-argument element constructor
    dj::read_write_container<std::vector<rw_probe_type> > objs;

    objs.emplace_back(7, 'q');
    objs.emplace_back(-3, 'z');

    D_RW_CHECK(objs.size() == 2u);
    D_RW_CHECK(objs[0].a == 7);
    D_RW_CHECK(objs[0].b == 'q');
    D_RW_CHECK(objs[1].a == -3);
    D_RW_CHECK(objs[1].b == 'z');

    // the arguments are not transposed
    objs.emplace_back(1, 'a');
    D_RW_CHECK(objs.back().a == 1);
    D_RW_CHECK(objs.back().b == 'a');

    // no arguments at all constructs a default element
    objs.emplace_back();
    D_RW_CHECK(objs.size() == 4u);
    D_RW_CHECK(objs.back().a == 0);
    D_RW_CHECK(objs.back().b == '\0');

    // a node container
    dj::read_write_container<std::list<rw_probe_type> > l;

    l.emplace_back(5, 'm');
    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front().a == 5);
    D_RW_CHECK(l.front().b == 'm');

    return true;
}

/*
tests_read_write_pop_back_removes_last
  removal from the end.
  Tests the following:
  - pop_back removes exactly the final element
  - the size drops by one and the remaining elements are untouched
  - popping to empty leaves the handle reporting empty
*/
bool
tests_read_write_pop_back_removes_last()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    h.push_back(2);
    h.push_back(3);

    h.pop_back();
    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h.back() == 2);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);

    h.pop_back();
    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(h.front() == 1);
    D_RW_CHECK(h.back() == 1);

    // to empty
    h.pop_back();
    D_RW_CHECK(h.size() == 0u);
    D_RW_CHECK(h.empty());
    D_RW_CHECK(h.begin() == h.end());

    // a node container
    dj::read_write_container<std::list<int> > l(std::size_t(2), 4);

    l.pop_back();
    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front() == 4);

    return true;
}

/*
tests_read_write_pop_front_removes_first
  removal from the front.
  Tests the following:
  - pop_front removes exactly the first element
  - the remaining elements keep their order
  - it is available only for the containers that offer it
*/
bool
tests_read_write_pop_front_removes_first()
{
    dj::read_write_container<std::deque<int> > d;

    d.push_back(1);
    d.push_back(2);
    d.push_back(3);

    d.pop_front();
    D_RW_CHECK(d.size() == 2u);
    D_RW_CHECK(d[0] == 2);
    D_RW_CHECK(d[1] == 3);
    D_RW_CHECK(d.front() == 2);

    d.pop_front();
    D_RW_CHECK(d.size() == 1u);
    D_RW_CHECK(d.front() == 3);

    // a node container
    dj::read_write_container<std::list<int> > l;

    l.push_back(7);
    l.push_back(8);
    l.pop_front();

    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front() == 8);

    // a vector does not offer it
    D_RW_CHECK(!rw_has_pop_front<std::vector<int> >::value);
    D_RW_CHECK(!rw_has_pop_front<
                   dj::read_write_container<std::vector<int> > >::value);
    D_RW_CHECK(rw_has_pop_front<
                   dj::read_write_container<std::list<int> > >::value);

    return true;
}

/*
tests_read_write_insert_places_at_position
  positional insertion.
  Tests the following:
  - insert places an element at the requested position
  - the elements after it shift back, keeping their order
  - inserting at begin and at end both behave as the container would
  - the call returns the container's own result
*/
bool
tests_read_write_insert_places_at_position()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    h.push_back(3);

    // insert in the middle
    h.insert(h.cbegin() + 1, 2);

    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);
    D_RW_CHECK(h[2] == 3);

    // at the front
    h.insert(h.cbegin(), 0);
    D_RW_CHECK(h.size() == 4u);
    D_RW_CHECK(h[0] == 0);
    D_RW_CHECK(h[1] == 1);

    // at the end
    h.insert(h.cend(), 4);
    D_RW_CHECK(h.size() == 5u);
    D_RW_CHECK(h.back() == 4);

    // the returned iterator names the inserted element
    std::vector<int>::iterator at = h.insert(h.cbegin(), -1);

    D_RW_CHECK(*at == -1);
    D_RW_CHECK(h.front() == -1);
    D_RW_CHECK(h.size() == 6u);

    // a node container
    dj::read_write_container<std::list<int> > l;

    l.push_back(2);
    l.insert(l.cbegin(), 1);

    D_RW_CHECK(l.size() == 2u);
    D_RW_CHECK(l.front() == 1);
    D_RW_CHECK(l.back() == 2);

    return true;
}

/*
tests_read_write_erase_removes_at_position
  positional removal.
  Tests the following:
  - erase removes the element at the requested position
  - the elements after it shift forward, keeping their order
  - erasing the first and the last element both behave correctly
  - the call returns the container's own result
*/
bool
tests_read_write_erase_removes_at_position()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    h.push_back(2);
    h.push_back(3);
    h.push_back(4);

    // the middle
    h.erase(h.cbegin() + 1);
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 3);
    D_RW_CHECK(h[2] == 4);

    // the front
    h.erase(h.cbegin());
    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h.front() == 3);

    // the returned iterator names what followed
    std::vector<int>::iterator after = h.erase(h.cbegin());

    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(*after == 4);
    D_RW_CHECK(h.front() == 4);

    // to empty
    h.erase(h.cbegin());
    D_RW_CHECK(h.empty());

    // a node container
    dj::read_write_container<std::list<int> > l;

    l.push_back(1);
    l.push_back(2);
    l.erase(l.cbegin());

    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front() == 2);

    return true;
}

/*
tests_read_write_clear_empties_the_container
  wholesale removal.
  Tests the following:
  - clear removes every element
  - the handle then reports empty with size zero and an empty range
  - clearing an already-empty container is harmless
  - the handle remains usable afterwards
*/
bool
tests_read_write_clear_empties_the_container()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(5), 7);

    D_RW_CHECK(h.size() == 5u);

    h.clear();

    D_RW_CHECK(h.size() == 0u);
    D_RW_CHECK(h.empty());
    D_RW_CHECK(h.begin() == h.end());
    D_RW_CHECK(h.cbegin() == h.cend());

    // clearing again is harmless
    h.clear();
    D_RW_CHECK(h.empty());

    // and the handle still works
    h.push_back(1);
    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(h[0] == 1);

    // a node container and a character container
    dj::read_write_container<std::list<int> > l(std::size_t(3), 1);

    l.clear();
    D_RW_CHECK(l.empty());

    dj::read_write_container<std::string> s(std::size_t(4), 'a');

    s.clear();
    D_RW_CHECK(s.empty());
    D_RW_CHECK(s.size() == 0u);

    return true;
}

/*
tests_read_write_resize_changes_the_count
  growing and shrinking.
  Tests the following:
  - resize to a larger count appends value-initialised elements
  - resize to a smaller count drops the trailing elements
  - the surviving elements keep their values
  - the two-argument form fills with the given value
*/
bool
tests_read_write_resize_changes_the_count()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    h.push_back(2);

    // grow: the new elements are value-initialised
    h.resize(4);
    D_RW_CHECK(h.size() == 4u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);
    D_RW_CHECK(h[2] == 0);
    D_RW_CHECK(h[3] == 0);

    // shrink: the trailing elements go
    h.resize(1);
    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h.back() == 1);

    // to zero
    h.resize(0);
    D_RW_CHECK(h.empty());

    // the two-argument form fills with the given value
    dj::read_write_container<std::vector<int> > f;

    f.resize(3, 9);
    D_RW_CHECK(f.size() == 3u);
    D_RW_CHECK(f[0] == 9);
    D_RW_CHECK(f[2] == 9);

    // a node container
    dj::read_write_container<std::list<int> > l;

    l.resize(2);
    D_RW_CHECK(l.size() == 2u);

    // a fixed-size container has no resize, and neither does its handle
    D_RW_CHECK(!rw_has_resize<
                   dj::read_write_container<std::array<int, 3> > >::value);

    return true;
}

/*
tests_read_write_reserve_affects_capacity_only
  the capacity hint.
  Tests the following:
  - reserve does not change the element count or the element values
  - the handle remains usable and the elements remain readable afterwards
  - it is available only for the containers that offer it
*/
bool
tests_read_write_reserve_affects_capacity_only()
{
    dj::read_write_container<std::vector<int> > h;

    h.push_back(1);
    h.push_back(2);

    const std::size_t before = h.size();

    h.reserve(100);

    // the count and the values are untouched
    D_RW_CHECK(h.size() == before);
    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 2);
    D_RW_CHECK(!h.empty());

    // and the handle still appends correctly
    h.push_back(3);
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[2] == 3);

    // reserving less than the current size is harmless
    h.reserve(1);
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[0] == 1);

    // a string offers it too
    dj::read_write_container<std::string> s;

    s.push_back('a');
    s.reserve(50);
    D_RW_CHECK(s.size() == 1u);
    D_RW_CHECK(s[0] == 'a');

    // a list does not, and neither does its handle
    D_RW_CHECK(!rw_has_reserve<std::list<int> >::value);
    D_RW_CHECK(!rw_has_reserve<
                   dj::read_write_container<std::list<int> > >::value);
    D_RW_CHECK(!rw_has_reserve<
                   dj::read_write_container<std::deque<int> > >::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
