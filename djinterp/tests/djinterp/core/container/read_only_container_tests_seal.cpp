/*******************************************************************************
* djinterp [core]                             read_only_container_tests_seal.cpp
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests_seal.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_only_exposes_no_mutator
  the absence half of the seal.
  Tests the following:
  - not one of the ten probed mutating members is present on the handle
  - the roll-up predicate agrees, so a member added later without a
    matching detector still trips this body
  - the observing members ARE present, so the absence is selective rather
    than the detectors simply failing on everything
*/
bool
tests_read_only_exposes_no_mutator()
{
    typedef dj::read_only_container<std::vector<int> > handle;

    const ro_caps c = ro_probe<handle>();

    // every mutator, named individually
    D_RO_CHECK(!c.push_back);
    D_RO_CHECK(!c.push_front);
    D_RO_CHECK(!c.emplace_back);
    D_RO_CHECK(!c.pop_back);
    D_RO_CHECK(!c.pop_front);
    D_RO_CHECK(!c.clear);
    D_RO_CHECK(!c.resize);
    D_RO_CHECK(!c.reserve);
    D_RO_CHECK(!c.insert);
    D_RO_CHECK(!c.erase);

    // and the roll-up
    D_RO_CHECK(!ro_any_mutator(c));

    // the detectors are not simply failing on everything: the observers of
    // the same handle are found
    D_RO_CHECK(c.size);
    D_RO_CHECK(c.empty);
    D_RO_CHECK(c.subscript);
    D_RO_CHECK(c.front);
    D_RO_CHECK(c.back);
    D_RO_CHECK(c.data);
    D_RO_CHECK(c.begin);
    D_RO_CHECK(c.end);
    D_RO_CHECK(c.cbegin);
    D_RO_CHECK(c.cend);

    return true;
}

/*
tests_read_only_seal_holds_for_every_container
  the absence, across container types.
  Tests the following:
  - no mutator appears whatever container is wrapped
  - this includes containers that offer the full mutating surface, so the
    seal is a property of the wrapper rather than of what it holds
  - it also holds for a container that offers almost no mutators anyway
*/
bool
tests_read_only_seal_holds_for_every_container()
{
    // a vector offers push_back, pop_back, insert, erase, clear, resize
    // and reserve; none of them survives the wrapping
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::vector<int> > >()));

    // a list adds push_front and pop_front
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::list<int> > >()));

    // a deque offers both ends and random access
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::deque<int> > >()));

    // a string is a full sequence container too
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::string> >()));

    // an array has almost no mutators to begin with
    D_RO_CHECK(!ro_any_mutator(
                   ro_probe<dj::read_only_container<std::array<int, 3> > >()));

    // the underlying containers really do have those mutators, which is
    // what makes the assertions above meaningful
    D_RO_CHECK(ro_any_mutator(ro_probe<std::vector<int> >()));
    D_RO_CHECK(ro_any_mutator(ro_probe<std::list<int> >()));
    D_RO_CHECK(ro_any_mutator(ro_probe<std::deque<int> >()));
    D_RO_CHECK(ro_has_push_front<std::list<int> >::value);
    D_RO_CHECK(ro_has_reserve<std::vector<int> >::value);

    return true;
}

/*
tests_read_only_subscript_returns_const_reference
  the constness half of the seal, at the subscript.
  Tests the following:
  - subscript yields a reference to const even on a NON-const handle, since
    the const overload is the only one there is
  - nothing can therefore be assigned through the returned value
  - the same holds through a const handle
*/
bool
tests_read_only_subscript_returns_const_reference()
{
    dj::read_only_container<std::vector<int> > h(std::size_t(3), 5);

    // a non-const handle still yields a const reference
    typedef decltype(h[0]) sub_result;

    D_RO_CHECK(std::is_reference<sub_result>::value);
    D_RO_CHECK(ro_const_result<sub_result>::value);
    D_RO_CHECK((std::is_same<sub_result, const int&>::value));

    // which means it is not assignable
    D_RO_CHECK(!std::is_assignable<sub_result, int>::value);

    // through a const handle, likewise
    const dj::read_only_container<std::vector<int> >& ch = h;

    D_RO_CHECK(ro_const_result<decltype(ch[0])>::value);
    D_RO_CHECK((std::is_same<decltype(ch[0]), const int&>::value));

    // and for a character container
    dj::read_only_container<std::string> s(std::size_t(2), 'a');

    D_RO_CHECK(ro_const_result<decltype(s[0])>::value);
    D_RO_CHECK(!std::is_assignable<decltype(s[0]), char>::value);

    // the sibling overlay does NOT behave this way, which is what makes the
    // restriction observable rather than incidental
    dj::read_write_container<std::vector<int> > rw(std::size_t(3), 5);

    D_RO_CHECK(!ro_const_result<decltype(rw[0])>::value);
    D_RO_CHECK(std::is_assignable<decltype(rw[0]), int>::value);

    return true;
}

/*
tests_read_only_front_and_back_return_const
  the constness of the end-element accessors.
  Tests the following:
  - front and back yield references to const on a non-const handle
  - neither is assignable through
  - this holds for a node container as well as a contiguous one
*/
bool
tests_read_only_front_and_back_return_const()
{
    dj::read_only_container<std::vector<int> > h(std::size_t(3), 5);

    D_RO_CHECK(ro_const_result<decltype(h.front())>::value);
    D_RO_CHECK(ro_const_result<decltype(h.back())>::value);
    D_RO_CHECK((std::is_same<decltype(h.front()), const int&>::value));
    D_RO_CHECK((std::is_same<decltype(h.back()), const int&>::value));

    D_RO_CHECK(!std::is_assignable<decltype(h.front()), int>::value);
    D_RO_CHECK(!std::is_assignable<decltype(h.back()), int>::value);

    // a node container
    dj::read_only_container<std::list<int> > l(std::size_t(2), 1);

    D_RO_CHECK(ro_const_result<decltype(l.front())>::value);
    D_RO_CHECK(ro_const_result<decltype(l.back())>::value);
    D_RO_CHECK(!std::is_assignable<decltype(l.front()), int>::value);

    // a fixed-size container
    dj::read_only_container<std::array<int, 2> > a;

    D_RO_CHECK(ro_const_result<decltype(a.front())>::value);
    D_RO_CHECK(ro_const_result<decltype(a.back())>::value);

    // again, the read_write sibling differs
    dj::read_write_container<std::list<int> > rw(std::size_t(2), 1);

    D_RO_CHECK(!ro_const_result<decltype(rw.front())>::value);
    D_RO_CHECK(std::is_assignable<decltype(rw.front()), int>::value);

    return true;
}

/*
tests_read_only_data_returns_pointer_to_const
  the constness of the raw-storage accessor.
  Tests the following:
  - data yields a pointer to const, closing the direct route into storage
  - the pointee is not assignable through
  - the sibling overlay yields a pointer to non-const from the same
    container, so the qualification is the wrapper's doing
*/
bool
tests_read_only_data_returns_pointer_to_const()
{
    dj::read_only_container<std::vector<int> > h(std::size_t(3), 5);

    typedef decltype(h.data()) data_result;

    D_RO_CHECK(std::is_pointer<data_result>::value);
    D_RO_CHECK(ro_const_pointee<data_result>::value);
    D_RO_CHECK((std::is_same<data_result, const int*>::value));

    // the pointee cannot be written
    D_RO_CHECK(!std::is_assignable<decltype(*h.data()), int>::value);

    // a character container
    dj::read_only_container<std::string> s(std::size_t(2), 'a');

    D_RO_CHECK(ro_const_pointee<decltype(s.data())>::value);

    // a fixed-size container
    dj::read_only_container<std::array<int, 2> > a;

    D_RO_CHECK(ro_const_pointee<decltype(a.data())>::value);

    // the read_write sibling over the same container does not qualify it
    dj::read_write_container<std::vector<int> > rw(std::size_t(3), 5);

    D_RO_CHECK(!ro_const_pointee<decltype(rw.data())>::value);
    D_RO_CHECK((std::is_same<decltype(rw.data()), int*>::value));
    D_RO_CHECK(std::is_assignable<decltype(*rw.data()), int>::value);

    return true;
}

/*
tests_read_only_iterators_are_const_iterators
  the constness of the iteration range.
  Tests the following:
  - begin and end yield const_iterator on a non-const handle, closing the
    route through a dereferenced iterator
  - dereferencing one is not assignable through
  - cbegin and cend agree, as they must
*/
bool
tests_read_only_iterators_are_const_iterators()
{
    dj::read_only_container<std::vector<int> > h(std::size_t(3), 5);

    D_RO_CHECK((std::is_same<decltype(h.begin()),
                             std::vector<int>::const_iterator>::value));
    D_RO_CHECK((std::is_same<decltype(h.end()),
                             std::vector<int>::const_iterator>::value));
    D_RO_CHECK((std::is_same<decltype(h.cbegin()),
                             std::vector<int>::const_iterator>::value));
    D_RO_CHECK((std::is_same<decltype(h.cend()),
                             std::vector<int>::const_iterator>::value));

    // not the mutable iterator
    D_RO_CHECK(!(std::is_same<decltype(h.begin()),
                              std::vector<int>::iterator>::value));

    // dereferencing yields a const reference
    D_RO_CHECK(ro_const_result<decltype(*h.begin())>::value);
    D_RO_CHECK(!std::is_assignable<decltype(*h.begin()), int>::value);

    // a node container behaves the same way
    dj::read_only_container<std::list<int> > l(std::size_t(2), 1);

    D_RO_CHECK((std::is_same<decltype(l.begin()),
                             std::list<int>::const_iterator>::value));
    D_RO_CHECK(!std::is_assignable<decltype(*l.begin()), int>::value);

    // while the read_write sibling hands out a mutable iterator
    dj::read_write_container<std::vector<int> > rw(std::size_t(3), 5);

    D_RO_CHECK((std::is_same<decltype(rw.begin()),
                             std::vector<int>::iterator>::value));
    D_RO_CHECK(std::is_assignable<decltype(*rw.begin()), int>::value);

    return true;
}

/*
tests_read_only_never_yields_the_container
  the escape-hatch check.
  Tests the following:
  - no observer hands back the held container itself, by value, reference
    or pointer - which would return every mutator at a stroke
  - each observer's return type is an element reference, an iterator, a
    pointer to a const element, or a plain count
*/
bool
tests_read_only_never_yields_the_container()
{
    typedef dj::read_only_container<std::vector<int> > handle;
    typedef std::vector<int>                           held;

    dj::read_only_container<std::vector<int> > h(std::size_t(2), 1);

    // none of the observers returns the container in any form
    D_RO_CHECK(!(std::is_same<decltype(h[0]), held&>::value));
    D_RO_CHECK(!(std::is_same<decltype(h[0]), const held&>::value));
    D_RO_CHECK(!(std::is_same<decltype(h.front()), held&>::value));
    D_RO_CHECK(!(std::is_same<decltype(h.back()), held&>::value));
    D_RO_CHECK(!(std::is_same<decltype(h.data()), held*>::value));
    D_RO_CHECK(!(std::is_same<decltype(h.data()), const held*>::value));

    // what they DO return
    D_RO_CHECK((std::is_same<decltype(h[0]), const int&>::value));
    D_RO_CHECK((std::is_same<decltype(h.data()), const int*>::value));
    D_RO_CHECK((std::is_same<decltype(h.size()),
                             held::size_type>::value));
    D_RO_CHECK((std::is_same<decltype(h.empty()), bool>::value));

    // the container_type typedef names the held type but is only a type;
    // no member returns a value of it
    D_RO_CHECK((std::is_same<handle::container_type, held>::value));

    // and the handle is not convertible to the container it holds
    D_RO_CHECK(!(std::is_convertible<handle, held>::value));
    D_RO_CHECK(!(std::is_convertible<handle, held&>::value));
    D_RO_CHECK(!(std::is_convertible<handle, const held&>::value));

    return true;
}

/*
tests_read_only_seal_is_independent_of_container
  the overlay as a restriction on the HANDLE.
  Tests the following:
  - what the underlying container would permit is irrelevant to what the
    handle permits: the same container yields a sealed handle here and an
    unsealed one under read_write
  - the difference is entirely the wrapper's, since the held type is the
    same in both cases
*/
bool
tests_read_only_seal_is_independent_of_container()
{
    typedef std::vector<int> held;

    const ro_caps raw = ro_probe<held>();
    const ro_caps ro  = ro_probe<dj::read_only_container<held> >();
    const ro_caps rw  = ro_probe<dj::read_write_container<held> >();

    // the container itself is fully mutable
    D_RO_CHECK(ro_any_mutator(raw));
    D_RO_CHECK(raw.push_back);
    D_RO_CHECK(raw.clear);

    // read_write passes that through
    D_RO_CHECK(ro_any_mutator(rw));
    D_RO_CHECK(rw.push_back);
    D_RO_CHECK(rw.clear);

    // read_only does not, from the very same container type
    D_RO_CHECK(!ro_any_mutator(ro));
    D_RO_CHECK(!ro.push_back);
    D_RO_CHECK(!ro.clear);

    // both overlays hold the identical type
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<held>::container_type,
                    dj::read_write_container<held>::container_type>::value));

    // and both observe the same surface; only the write side differs
    D_RO_CHECK(ro_observers_match(ro, rw));

    return true;
}

/*
tests_read_only_value_type_is_not_itself_const
  where the seal actually lives.
  Tests the following:
  - the element type published by the handle is unqualified, because the
    restriction is carried by the returned references rather than by the
    element type
  - it matches the held container's own value_type exactly
  - the returned references are const even though the element type is not,
    which is the distinction this body exists to pin
*/
bool
tests_read_only_value_type_is_not_itself_const()
{
    typedef dj::read_only_container<std::vector<int> > handle;

    // the element type is plain
    D_RO_CHECK(!std::is_const<handle::value_type>::value);
    D_RO_CHECK(!std::is_reference<handle::value_type>::value);
    D_RO_CHECK((std::is_same<handle::value_type, int>::value));
    D_RO_CHECK((std::is_same<handle::value_type,
                             std::vector<int>::value_type>::value));

    // yet the references handed out are const
    dj::read_only_container<std::vector<int> > h(std::size_t(2), 1);

    D_RO_CHECK(ro_const_result<decltype(h[0])>::value);
    D_RO_CHECK(ro_const_result<decltype(h.front())>::value);
    D_RO_CHECK(ro_const_pointee<decltype(h.data())>::value);

    // so a caller may still declare and copy a value of the element type
    handle::value_type copy = h[0];

    copy = 42;
    D_RO_CHECK(copy == 42);
    D_RO_CHECK(h[0] == 1);

    return true;
}

NS_END  // testing
NS_END  // djinterp
