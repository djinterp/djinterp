/*******************************************************************************
* djinterp [core]                     read_write_container_tests_observation.cpp
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests_observation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// rw_sample
//   function: a small, known vector the observation tests share.
static std::vector<int>
rw_sample()
{
    std::vector<int> v;

    v.push_back(11);
    v.push_back(22);
    v.push_back(33);
    v.push_back(44);

    return v;
}


/*
tests_read_write_size_and_empty
  the two count observers.
  Tests the following:
  - size reports the held container's element count
  - empty agrees with size being zero
  - both are const members, answering on a const handle
*/
bool
tests_read_write_size_and_empty()
{
    const dj::read_write_container<std::vector<int> > h(rw_sample());

    D_RW_CHECK(h.size() == 4u);
    D_RW_CHECK(!h.empty());
    D_RW_CHECK(h.empty() == (h.size() == 0u));

    const dj::read_write_container<std::vector<int> > e;

    D_RW_CHECK(e.size() == 0u);
    D_RW_CHECK(e.empty());

    // a node container, where size is not a pointer subtraction
    const dj::read_write_container<std::list<int> > l(std::size_t(3), 9);

    D_RW_CHECK(l.size() == 3u);
    D_RW_CHECK(!l.empty());

    // a fixed-size container reports its own size
    const dj::read_write_container<std::array<int, 2> > a;

    D_RW_CHECK(a.size() == 2u);
    D_RW_CHECK(!a.empty());

    return true;
}

/*
tests_read_write_const_subscript_reads
  the const half of indexed access.
  Tests the following:
  - through a const handle the subscript reads every element
  - the returned reference is const-qualified, since that overload is the
    only one available on a const handle
  - the values match the container the handle was built from
*/
bool
tests_read_write_const_subscript_reads()
{
    const std::vector<int>                            src = rw_sample();
    const dj::read_write_container<std::vector<int> > h(src);

    D_RW_CHECK(h[0] == 11);
    D_RW_CHECK(h[1] == 22);
    D_RW_CHECK(h[3] == 44);

    // the const overload is selected, and it is const-qualified
    D_RW_CHECK(rw_const_result<decltype(h[0])>::value);
    D_RW_CHECK((std::is_same<decltype(h[0]), const int&>::value));
    D_RW_CHECK(!std::is_assignable<decltype(h[0]), int>::value);

    // every index agrees with the source
    std::size_t i = 0;

    for (i = 0; i < h.size(); ++i)
    {
        if (h[i] != src[i])
        {
            std::printf("    [FAIL] index %u: got %d, expected %d\n",
                        static_cast<unsigned int>(i), h[i], src[i]);

            return false;
        }
    }

    return true;
}

/*
tests_read_write_const_front_and_back
  the const end-element accessors.
  Tests the following:
  - front names the first element and back the last, through a const handle
  - each agrees with the corresponding subscript
  - both return const references on a const handle
*/
bool
tests_read_write_const_front_and_back()
{
    const dj::read_write_container<std::vector<int> > h(rw_sample());

    D_RW_CHECK(h.front() == 11);
    D_RW_CHECK(h.back() == 44);
    D_RW_CHECK(h.front() == h[0]);
    D_RW_CHECK(h.back() == h[h.size() - 1u]);
    D_RW_CHECK(&h.front() == &h[0]);

    D_RW_CHECK(rw_const_result<decltype(h.front())>::value);
    D_RW_CHECK(rw_const_result<decltype(h.back())>::value);

    // a node container has both without a subscript
    const dj::read_write_container<std::list<int> > l(std::size_t(2), 3);

    D_RW_CHECK(l.front() == 3);
    D_RW_CHECK(l.back() == 3);
    D_RW_CHECK(rw_const_result<decltype(l.front())>::value);

    return true;
}

/*
tests_read_write_const_data_and_iteration
  the const storage pointer and range.
  Tests the following:
  - data addresses the handle's own storage through a const handle and is
    const-qualified there
  - begin and end span exactly the held elements, in order
  - the distance between them matches size
*/
bool
tests_read_write_const_data_and_iteration()
{
    const std::vector<int>                            src = rw_sample();
    const dj::read_write_container<std::vector<int> > h(src);

    D_RW_CHECK(h.data() != 0);
    D_RW_CHECK(*h.data() == 11);
    D_RW_CHECK(h.data()[3] == 44);
    D_RW_CHECK(h.data() == &h[0]);
    D_RW_CHECK(rw_const_pointee<decltype(h.data())>::value);

    // iteration
    std::size_t                      n = 0;
    std::vector<int>::const_iterator it;

    for (it = h.begin(); it != h.end(); ++it)
    {
        if (*it != src[n])
        {
            std::printf("    [FAIL] position %u: got %d, expected %d\n",
                        static_cast<unsigned int>(n), *it, src[n]);

            return false;
        }

        ++n;
    }

    D_RW_CHECK(n == h.size());
    D_RW_CHECK((std::is_same<decltype(h.begin()),
                             std::vector<int>::const_iterator>::value));

    // an empty handle yields an empty range
    const dj::read_write_container<std::vector<int> > e;

    D_RW_CHECK(e.begin() == e.end());

    return true;
}

/*
tests_read_write_cbegin_cend_are_const
  the explicitly const iterators.
  Tests the following:
  - cbegin and cend yield const iterators even from a NON-const handle,
    which is the point of having them alongside begin and end
  - they describe the same range as the non-const pair does
  - the range is empty exactly when the container is
*/
bool
tests_read_write_cbegin_cend_are_const()
{
    dj::read_write_container<std::vector<int> > h(rw_sample());

    // const iterators from a non-const handle
    D_RW_CHECK((std::is_same<decltype(h.cbegin()),
                             std::vector<int>::const_iterator>::value));
    D_RW_CHECK((std::is_same<decltype(h.cend()),
                             std::vector<int>::const_iterator>::value));
    D_RW_CHECK(rw_const_result<decltype(*h.cbegin())>::value);
    D_RW_CHECK(!std::is_assignable<decltype(*h.cbegin()), int>::value);

    // while begin from the same handle is mutable
    D_RW_CHECK((std::is_same<decltype(h.begin()),
                             std::vector<int>::iterator>::value));

    // the two describe the same range
    std::size_t n = 0;
    std::vector<int>::const_iterator it;

    for (it = h.cbegin(); it != h.cend(); ++it)
    {
        ++n;
    }

    D_RW_CHECK(n == h.size());
    D_RW_CHECK(*h.cbegin() == h.front());

    // empty
    dj::read_write_container<std::vector<int> > e;

    D_RW_CHECK(e.cbegin() == e.cend());

    return true;
}

/*
tests_read_write_const_handle_keeps_observers
  what a const handle retains.
  Tests the following:
  - every observing member is callable on a const handle
  - the answers match those from a non-const handle
  - the mutating members are NOT callable on a const handle, which is what
    makes constness meaningful here
*/
bool
tests_read_write_const_handle_keeps_observers()
{
    dj::read_write_container<std::vector<int> >        mutable_handle(
        rw_sample());
    const dj::read_write_container<std::vector<int> >& h = mutable_handle;

    // the whole observing surface answers
    D_RW_CHECK(h.size() == 4u);
    D_RW_CHECK(!h.empty());
    D_RW_CHECK(h[1] == 22);
    D_RW_CHECK(h.front() == 11);
    D_RW_CHECK(h.back() == 44);
    D_RW_CHECK(h.data()[0] == 11);
    D_RW_CHECK(h.begin() != h.end());
    D_RW_CHECK(h.cbegin() != h.cend());

    // and agrees with the non-const handle
    D_RW_CHECK(h.size() == mutable_handle.size());
    D_RW_CHECK(h[2] == mutable_handle[2]);
    D_RW_CHECK(h.data() == mutable_handle.data());

    // a const handle offers no mutating member
    typedef const dj::read_write_container<std::vector<int> > const_handle;

    D_RW_CHECK(!rw_has_push_back<const_handle>::value);
    D_RW_CHECK(!rw_has_clear<const_handle>::value);
    D_RW_CHECK(!rw_has_pop_back<const_handle>::value);
    D_RW_CHECK(!rw_has_resize<const_handle>::value);

    // while the non-const one does
    typedef dj::read_write_container<std::vector<int> > handle;

    D_RW_CHECK(rw_has_push_back<handle>::value);
    D_RW_CHECK(rw_has_clear<handle>::value);

    return true;
}

/*
tests_read_write_observers_track_mutations
  the two halves addressing one container.
  Tests the following:
  - each mutation is immediately visible through the observers
  - size, front, back and the iteration range all move together
  - the handle is not caching a stale view of its container
*/
bool
tests_read_write_observers_track_mutations()
{
    dj::read_write_container<std::vector<int> > h;

    D_RW_CHECK(h.empty());
    D_RW_CHECK(h.size() == 0u);
    D_RW_CHECK(h.begin() == h.end());

    h.push_back(1);
    D_RW_CHECK(!h.empty());
    D_RW_CHECK(h.size() == 1u);
    D_RW_CHECK(h.front() == 1);
    D_RW_CHECK(h.back() == 1);
    D_RW_CHECK(h.begin() != h.end());

    h.push_back(2);
    h.push_back(3);
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h.front() == 1);
    D_RW_CHECK(h.back() == 3);
    D_RW_CHECK(h[1] == 2);

    // a write through the element surface is observed too
    h[1] = 20;
    D_RW_CHECK(h[1] == 20);
    D_RW_CHECK(*(h.begin() + 1) == 20);

    // removal
    h.pop_back();
    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h.back() == 20);

    h.clear();
    D_RW_CHECK(h.empty());
    D_RW_CHECK(h.size() == 0u);
    D_RW_CHECK(h.begin() == h.end());

    return true;
}

NS_END  // testing
NS_END  // djinterp
