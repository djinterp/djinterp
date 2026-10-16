/*******************************************************************************
* djinterp [core]                  read_write_container_tests_element_access.cpp
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests_element_access.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_write_subscript_returns_mutable_ref
  the write side of indexed access.
  Tests the following:
  - subscript on a non-const handle yields a reference to NON-const
  - assigning through it changes the held element
  - the change is visible through every other observer
*/
bool
tests_read_write_subscript_returns_mutable_ref()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(3), 5);

    typedef decltype(h[0]) sub_result;

    D_RW_CHECK(std::is_reference<sub_result>::value);
    D_RW_CHECK(!rw_const_result<sub_result>::value);
    D_RW_CHECK((std::is_same<sub_result, int&>::value));
    D_RW_CHECK(std::is_assignable<sub_result, int>::value);

    // and the assignment takes effect
    h[0] = 10;
    h[2] = 30;

    D_RW_CHECK(h[0] == 10);
    D_RW_CHECK(h[1] == 5);
    D_RW_CHECK(h[2] == 30);
    D_RW_CHECK(h.front() == 10);
    D_RW_CHECK(h.back() == 30);
    D_RW_CHECK(*h.data() == 10);

    // a character container
    dj::read_write_container<std::string> s(std::size_t(2), 'a');

    s[1] = 'b';
    D_RW_CHECK(s[0] == 'a');
    D_RW_CHECK(s[1] == 'b');

    // a fixed-size container is writable through the subscript too
    dj::read_write_container<std::array<int, 2> > a;

    a[0] = 7;
    D_RW_CHECK(a[0] == 7);

    return true;
}

/*
tests_read_write_front_and_back_are_mutable
  the write side of the end-element accessors.
  Tests the following:
  - front and back yield references to non-const on a non-const handle
  - assigning through either changes the corresponding element
  - on a single-element container the two name the same object
*/
bool
tests_read_write_front_and_back_are_mutable()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(3), 5);

    D_RW_CHECK(!rw_const_result<decltype(h.front())>::value);
    D_RW_CHECK(!rw_const_result<decltype(h.back())>::value);
    D_RW_CHECK((std::is_same<decltype(h.front()), int&>::value));
    D_RW_CHECK(std::is_assignable<decltype(h.front()), int>::value);

    h.front() = 1;
    h.back()  = 9;

    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[1] == 5);
    D_RW_CHECK(h[2] == 9);

    // a node container, which has no subscript to cross-check against
    dj::read_write_container<std::list<int> > l(std::size_t(2), 3);

    D_RW_CHECK(!rw_const_result<decltype(l.front())>::value);
    l.front() = 4;
    l.back()  = 6;
    D_RW_CHECK(l.front() == 4);
    D_RW_CHECK(l.back() == 6);

    // one element: front and back are the same object
    dj::read_write_container<std::vector<int> > one(std::size_t(1), 0);

    one.front() = 8;
    D_RW_CHECK(one.back() == 8);
    D_RW_CHECK(&one.front() == &one.back());

    return true;
}

/*
tests_read_write_data_returns_mutable_pointer
  the write side of the raw-storage accessor.
  Tests the following:
  - data yields a pointer to non-const on a non-const handle
  - writing through it reaches the held container
  - the pointer addresses the same storage the subscript names
*/
bool
tests_read_write_data_returns_mutable_pointer()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(3), 5);

    typedef decltype(h.data()) data_result;

    D_RW_CHECK(std::is_pointer<data_result>::value);
    D_RW_CHECK(!rw_const_pointee<data_result>::value);
    D_RW_CHECK((std::is_same<data_result, int*>::value));
    D_RW_CHECK(std::is_assignable<decltype(*h.data()), int>::value);

    // it addresses the same storage
    D_RW_CHECK(h.data() == &h[0]);
    D_RW_CHECK(h.data() == &h.front());

    // writing through it reaches the container
    *h.data()     = 1;
    h.data()[2]   = 3;

    D_RW_CHECK(h[0] == 1);
    D_RW_CHECK(h[2] == 3);
    D_RW_CHECK(h.front() == 1);
    D_RW_CHECK(h.back() == 3);

    // a character container
    dj::read_write_container<std::string> s(std::size_t(2), 'a');

    s.data()[0] = 'z';
    D_RW_CHECK(s[0] == 'z');

    return true;
}

/*
tests_read_write_iterators_are_mutable
  the write side of the iteration range.
  Tests the following:
  - begin and end yield ordinary iterators on a non-const handle
  - dereferencing one is assignable through
  - a write made during iteration is visible afterwards
*/
bool
tests_read_write_iterators_are_mutable()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(3), 5);

    D_RW_CHECK((std::is_same<decltype(h.begin()),
                             std::vector<int>::iterator>::value));
    D_RW_CHECK((std::is_same<decltype(h.end()),
                             std::vector<int>::iterator>::value));
    D_RW_CHECK(!rw_const_result<decltype(*h.begin())>::value);
    D_RW_CHECK(std::is_assignable<decltype(*h.begin()), int>::value);

    // write through the range
    std::vector<int>::iterator it;
    int                        n = 0;

    for (it = h.begin(); it != h.end(); ++it)
    {
        *it = n;
        ++n;
    }

    D_RW_CHECK(h[0] == 0);
    D_RW_CHECK(h[1] == 1);
    D_RW_CHECK(h[2] == 2);

    // a node container
    dj::read_write_container<std::list<int> > l(std::size_t(2), 0);
    std::list<int>::iterator                  lit;

    D_RW_CHECK((std::is_same<decltype(l.begin()),
                             std::list<int>::iterator>::value));

    for (lit = l.begin(); lit != l.end(); ++lit)
    {
        *lit = 7;
    }

    D_RW_CHECK(l.front() == 7);
    D_RW_CHECK(l.back() == 7);

    return true;
}

/*
tests_read_write_writes_reach_the_container
  the four write routes agreeing.
  Tests the following:
  - a write made through any of subscript, front/back, data or an iterator
    is visible through all of the others
  - all four address one container rather than four separate views
*/
bool
tests_read_write_writes_reach_the_container()
{
    dj::read_write_container<std::vector<int> > h(std::size_t(4), 0);

    // through the subscript
    h[0] = 1;
    D_RW_CHECK(h.front() == 1);
    D_RW_CHECK(*h.data() == 1);
    D_RW_CHECK(*h.begin() == 1);

    // through front
    h.front() = 2;
    D_RW_CHECK(h[0] == 2);
    D_RW_CHECK(*h.data() == 2);

    // through data
    h.data()[0] = 3;
    D_RW_CHECK(h[0] == 3);
    D_RW_CHECK(h.front() == 3);

    // through an iterator
    *h.begin() = 4;
    D_RW_CHECK(h[0] == 4);
    D_RW_CHECK(h.front() == 4);
    D_RW_CHECK(*h.data() == 4);

    // the last element, through back and through the subscript
    h.back() = 9;
    D_RW_CHECK(h[3] == 9);
    D_RW_CHECK(h.data()[3] == 9);

    h[3] = 8;
    D_RW_CHECK(h.back() == 8);

    // all four routes name the same address
    D_RW_CHECK(&h[0] == &h.front());
    D_RW_CHECK(&h[0] == h.data());
    D_RW_CHECK(&h[0] == &(*h.begin()));

    return true;
}

/*
tests_read_write_const_handle_narrows_to_const
  constness propagating through the handle.
  Tests the following:
  - the same accessors yield const references through a const handle
  - none of them is assignable through in that case
  - the handle's constness, not the container's, decides
*/
bool
tests_read_write_const_handle_narrows_to_const()
{
    dj::read_write_container<std::vector<int> >        m(std::size_t(3), 5);
    const dj::read_write_container<std::vector<int> >& h = m;

    D_RW_CHECK(rw_const_result<decltype(h[0])>::value);
    D_RW_CHECK(rw_const_result<decltype(h.front())>::value);
    D_RW_CHECK(rw_const_result<decltype(h.back())>::value);
    D_RW_CHECK(rw_const_pointee<decltype(h.data())>::value);
    D_RW_CHECK(rw_const_result<decltype(*h.begin())>::value);

    D_RW_CHECK(!std::is_assignable<decltype(h[0]), int>::value);
    D_RW_CHECK(!std::is_assignable<decltype(h.front()), int>::value);
    D_RW_CHECK(!std::is_assignable<decltype(*h.begin()), int>::value);

    // the same object through the non-const reference is writable
    D_RW_CHECK(!rw_const_result<decltype(m[0])>::value);
    D_RW_CHECK(std::is_assignable<decltype(m[0]), int>::value);

    m[0] = 42;
    D_RW_CHECK(h[0] == 42);

    // a const handle behaves like the read_only overlay on this point
    dj::read_only_container<std::vector<int> > ro(std::size_t(3), 5);

    D_RW_CHECK(rw_const_result<decltype(ro[0])>::value ==
               rw_const_result<decltype(h[0])>::value);

    return true;
}

/*
tests_read_write_both_overload_sets_coexist
  overload selection by handle constness.
  Tests the following:
  - the const and non-const overloads both exist and select correctly
  - a non-const handle picks the mutable overload, a const one the const
    overload, for each of the five doubled accessors
  - cbegin and cend are const-only and therefore unaffected
*/
bool
tests_read_write_both_overload_sets_coexist()
{
    dj::read_write_container<std::vector<int> >        m(std::size_t(2), 1);
    const dj::read_write_container<std::vector<int> >& c = m;

    // subscript
    D_RW_CHECK((std::is_same<decltype(m[0]), int&>::value));
    D_RW_CHECK((std::is_same<decltype(c[0]), const int&>::value));

    // front / back
    D_RW_CHECK((std::is_same<decltype(m.front()), int&>::value));
    D_RW_CHECK((std::is_same<decltype(c.front()), const int&>::value));
    D_RW_CHECK((std::is_same<decltype(m.back()), int&>::value));
    D_RW_CHECK((std::is_same<decltype(c.back()), const int&>::value));

    // data
    D_RW_CHECK((std::is_same<decltype(m.data()), int*>::value));
    D_RW_CHECK((std::is_same<decltype(c.data()), const int*>::value));

    // begin / end
    D_RW_CHECK((std::is_same<decltype(m.begin()),
                             std::vector<int>::iterator>::value));
    D_RW_CHECK((std::is_same<decltype(c.begin()),
                             std::vector<int>::const_iterator>::value));
    D_RW_CHECK((std::is_same<decltype(m.end()),
                             std::vector<int>::iterator>::value));
    D_RW_CHECK((std::is_same<decltype(c.end()),
                             std::vector<int>::const_iterator>::value));

    // cbegin / cend are const-only, so both handles agree
    D_RW_CHECK((std::is_same<decltype(m.cbegin()),
                             decltype(c.cbegin())>::value));
    D_RW_CHECK((std::is_same<decltype(m.cend()),
                             decltype(c.cend())>::value));

    // both overloads address the same element
    D_RW_CHECK(&m[0] == &c[0]);
    D_RW_CHECK(m.data() == c.data());

    return true;
}

NS_END  // testing
NS_END  // djinterp
