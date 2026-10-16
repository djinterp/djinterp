/*******************************************************************************
* djinterp [core]                      read_only_container_tests_observation.cpp
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests_observation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// ro_sample
//   function: a small, known vector the observation tests share.
static std::vector<int>
ro_sample()
{
    std::vector<int> v;

    v.push_back(11);
    v.push_back(22);
    v.push_back(33);
    v.push_back(44);

    return v;
}


/*
tests_read_only_size_and_empty
  the two count observers.
  Tests the following:
  - size reports the held container's element count
  - empty agrees with size being zero
  - both follow the container the handle was built from, not a default
*/
bool
tests_read_only_size_and_empty()
{
    const dj::read_only_container<std::vector<int> > h(ro_sample());

    D_RO_CHECK(h.size() == 4u);
    D_RO_CHECK(!h.empty());
    D_RO_CHECK(h.empty() == (h.size() == 0u));

    // an empty container
    const dj::read_only_container<std::vector<int> > e;

    D_RO_CHECK(e.size() == 0u);
    D_RO_CHECK(e.empty());
    D_RO_CHECK(e.empty() == (e.size() == 0u));

    // a single element
    const dj::read_only_container<std::vector<int> > one(std::size_t(1), 5);

    D_RO_CHECK(one.size() == 1u);
    D_RO_CHECK(!one.empty());

    // and a node container, where size is not a pointer subtraction
    const dj::read_only_container<std::list<int> > l(std::size_t(3), 9);

    D_RO_CHECK(l.size() == 3u);
    D_RO_CHECK(!l.empty());

    return true;
}

/*
tests_read_only_subscript_reads_elements
  indexed access.
  Tests the following:
  - every index from the first to the last reaches its own element
  - the values match what the container was built with
  - repeated reads are stable
*/
bool
tests_read_only_subscript_reads_elements()
{
    const std::vector<int>                           src = ro_sample();
    const dj::read_only_container<std::vector<int> > h(src);

    D_RO_CHECK(h[0] == 11);
    D_RO_CHECK(h[1] == 22);
    D_RO_CHECK(h[2] == 33);
    D_RO_CHECK(h[3] == 44);

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

    // reads are stable
    D_RO_CHECK(h[0] == h[0]);
    D_RO_CHECK(h[2] == 33);

    // and the same holds for a character container
    const dj::read_only_container<std::string> s(std::string("abc"));

    D_RO_CHECK(s[0] == 'a');
    D_RO_CHECK(s[2] == 'c');

    return true;
}

/*
tests_read_only_front_and_back
  the end-element observers.
  Tests the following:
  - front names the first element and back the last
  - each agrees with the corresponding subscript
  - on a single-element container the two name the same element
*/
bool
tests_read_only_front_and_back()
{
    const dj::read_only_container<std::vector<int> > h(ro_sample());

    D_RO_CHECK(h.front() == 11);
    D_RO_CHECK(h.back() == 44);
    D_RO_CHECK(h.front() == h[0]);
    D_RO_CHECK(h.back() == h[h.size() - 1u]);

    // they address the same objects the subscripts do
    D_RO_CHECK(&h.front() == &h[0]);
    D_RO_CHECK(&h.back() == &h[3]);

    // one element: front and back coincide
    const dj::read_only_container<std::vector<int> > one(std::size_t(1), 7);

    D_RO_CHECK(one.front() == 7);
    D_RO_CHECK(one.back() == 7);
    D_RO_CHECK(&one.front() == &one.back());

    // a node container has front and back but no subscript
    const dj::read_only_container<std::list<int> > l(std::size_t(2), 3);

    D_RO_CHECK(l.front() == 3);
    D_RO_CHECK(l.back() == 3);

    return true;
}

/*
tests_read_only_data_addresses_storage
  the raw-storage observer.
  Tests the following:
  - data addresses the handle's OWN storage, so the first element is
    reachable through it
  - the pointer is stable across calls on the same handle
  - it is not the address of the container the handle was built from
*/
bool
tests_read_only_data_addresses_storage()
{
    std::vector<int> src = ro_sample();

    const dj::read_only_container<std::vector<int> > h(src);

    D_RO_CHECK(h.data() != 0);
    D_RO_CHECK(*h.data() == 11);
    D_RO_CHECK(h.data()[1] == 22);
    D_RO_CHECK(h.data()[3] == 44);

    // it addresses the same object the subscript names
    D_RO_CHECK(h.data() == &h[0]);
    D_RO_CHECK(h.data() == &h.front());

    // stable across calls
    D_RO_CHECK(h.data() == h.data());

    // and distinct from the source's storage, since the handle holds a copy
    D_RO_CHECK(h.data() != src.data());

    // a character container's data spans its characters
    const dj::read_only_container<std::string> s(std::string("xyz"));

    D_RO_CHECK(s.data()[0] == 'x');
    D_RO_CHECK(s.data()[2] == 'z');

    return true;
}

/*
tests_read_only_iteration_visits_elements
  the iteration range.
  Tests the following:
  - begin and end span exactly the held elements
  - the elements are visited in order
  - the distance between the two matches size
*/
bool
tests_read_only_iteration_visits_elements()
{
    const std::vector<int>                           src = ro_sample();
    const dj::read_only_container<std::vector<int> > h(src);

    std::size_t                                 n = 0;
    std::vector<int>::const_iterator            it;

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

    D_RO_CHECK(n == h.size());
    D_RO_CHECK(n == 4u);

    // the first and last elements agree with the other observers
    D_RO_CHECK(*h.begin() == h.front());

    // a node container iterates too
    const dj::read_only_container<std::list<int> > l(std::size_t(3), 8);
    std::size_t                                    ln = 0;
    std::list<int>::const_iterator                 lit;

    for (lit = l.begin(); lit != l.end(); ++lit)
    {
        D_RO_CHECK(*lit == 8);
        ++ln;
    }

    D_RO_CHECK(ln == l.size());

    return true;
}

/*
tests_read_only_cbegin_cend_match_begin_end
  the explicitly const iterators.
  Tests the following:
  - cbegin and cend describe the same range as begin and end
  - the two pairs compare equal, since begin already yields a const iterator
  - the range is empty exactly when the container is
*/
bool
tests_read_only_cbegin_cend_match_begin_end()
{
    const dj::read_only_container<std::vector<int> > h(ro_sample());

    D_RO_CHECK(h.cbegin() == h.begin());
    D_RO_CHECK(h.cend() == h.end());
    D_RO_CHECK(*h.cbegin() == 11);

    // both pairs span the same count
    std::size_t                      n = 0;
    std::vector<int>::const_iterator it;

    for (it = h.cbegin(); it != h.cend(); ++it)
    {
        ++n;
    }

    D_RO_CHECK(n == h.size());

    // on an empty container both ranges are empty
    const dj::read_only_container<std::vector<int> > e;

    D_RO_CHECK(e.cbegin() == e.cend());
    D_RO_CHECK(e.begin() == e.end());
    D_RO_CHECK(e.cbegin() == e.begin());

    return true;
}

/*
tests_read_only_observers_work_on_const_handle
  const-callability.
  Tests the following:
  - every observer is a const member, so all of them are callable on a
    const handle
  - the answers are the same as on a non-const handle
*/
bool
tests_read_only_observers_work_on_const_handle()
{
    dj::read_only_container<std::vector<int> >        mutable_handle(
        ro_sample());
    const dj::read_only_container<std::vector<int> >& h = mutable_handle;

    // all ten observers, on a const handle
    D_RO_CHECK(h.size() == 4u);
    D_RO_CHECK(!h.empty());
    D_RO_CHECK(h[1] == 22);
    D_RO_CHECK(h.front() == 11);
    D_RO_CHECK(h.back() == 44);
    D_RO_CHECK(h.data()[0] == 11);
    D_RO_CHECK(h.begin() != h.end());
    D_RO_CHECK(h.cbegin() != h.cend());

    // and they agree with the non-const handle
    D_RO_CHECK(h.size() == mutable_handle.size());
    D_RO_CHECK(h[2] == mutable_handle[2]);
    D_RO_CHECK(h.front() == mutable_handle.front());
    D_RO_CHECK(h.back() == mutable_handle.back());
    D_RO_CHECK(h.data() == mutable_handle.data());
    D_RO_CHECK(h.begin() == mutable_handle.begin());

    return true;
}

/*
tests_read_only_observes_an_empty_container
  the zero-element case.
  Tests the following:
  - an empty handle reports empty with size zero
  - its iteration range is empty rather than malformed
  - the observers that are safe on an empty container do not misbehave
*/
bool
tests_read_only_observes_an_empty_container()
{
    const dj::read_only_container<std::vector<int> > e;

    D_RO_CHECK(e.size() == 0u);
    D_RO_CHECK(e.empty());
    D_RO_CHECK(e.begin() == e.end());
    D_RO_CHECK(e.cbegin() == e.cend());

    // the loop body is never entered
    std::size_t                      n = 0;
    std::vector<int>::const_iterator it;

    for (it = e.begin(); it != e.end(); ++it)
    {
        ++n;
    }

    D_RO_CHECK(n == 0u);

    // explicitly emptied sources behave the same way
    const dj::read_only_container<std::vector<int> > z(
        std::size_t(0), 0);

    D_RO_CHECK(z.empty());
    D_RO_CHECK(z.begin() == z.end());

    // as does an empty string and an empty list
    const dj::read_only_container<std::string>     s;
    const dj::read_only_container<std::list<int> > l;

    D_RO_CHECK(s.empty());
    D_RO_CHECK(l.empty());
    D_RO_CHECK(l.begin() == l.end());

    return true;
}

/*
tests_read_only_forwards_element_values_faithfully
  value transparency.
  Tests the following:
  - values pass through the wrapper unchanged, including negative ones and
    the extremes of the element type
  - a user-defined element type survives with all its members intact
*/
bool
tests_read_only_forwards_element_values_faithfully()
{
    std::vector<int> src;

    src.push_back(0);
    src.push_back(-1);
    src.push_back(2147483647);
    src.push_back(-2147483647 - 1);

    const dj::read_only_container<std::vector<int> > h(src);

    D_RO_CHECK(h[0] == 0);
    D_RO_CHECK(h[1] == -1);
    D_RO_CHECK(h[2] == 2147483647);
    D_RO_CHECK(h[3] == (-2147483647 - 1));

    // a user-defined element type
    std::vector<ro_probe_type> objs;

    objs.push_back(ro_probe_type(7, 'q'));
    objs.push_back(ro_probe_type(-3, 'z'));

    const dj::read_only_container<std::vector<ro_probe_type> > oh(objs);

    D_RO_CHECK(oh.size() == 2u);
    D_RO_CHECK(oh[0].a == 7);
    D_RO_CHECK(oh[0].b == 'q');
    D_RO_CHECK(oh[1].a == -3);
    D_RO_CHECK(oh[1].b == 'z');
    D_RO_CHECK(oh.front().a == 7);
    D_RO_CHECK(oh.back().b == 'z');

    // characters, including a NUL, survive
    std::string text("a");

    text.push_back('\0');
    text.push_back('b');

    const dj::read_only_container<std::string> s(text);

    D_RO_CHECK(s.size() == 3u);
    D_RO_CHECK(s[0] == 'a');
    D_RO_CHECK(s[1] == '\0');
    D_RO_CHECK(s[2] == 'b');

    return true;
}

NS_END  // testing
NS_END  // djinterp
