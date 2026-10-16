/*******************************************************************************
* djinterp [core]                      read_write_container_tests_adaptivity.cpp
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests_adaptivity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// rw_check_mirror
//   function: the shared body of the mirror assertions.  Compares the
// handle's detection row with the held container's and, on a mismatch, names
// every member they disagree on rather than merely returning false.
template<typename _Container>
static bool
rw_check_mirror(
    const char* _what
)
{
    const rw_caps wrapped =
        rw_probe<dj::read_write_container<_Container> >();
    const rw_caps raw = rw_probe<_Container>();

    if (!rw_rows_match(wrapped, raw))
    {
        rw_report_mismatch(_what, wrapped, raw);

        return false;
    }

    return true;
}


/*
tests_read_write_mirrors_the_container_exactly
  the wrapper's whole contract, as one invariant.
  Tests the following:
  - the handle's detection row equals the held container's on all twenty
    probed members, so the wrapper neither invents nor loses a member
  - the invariant holds across contiguous, node, double-ended, fixed-size
    and character containers
  - a mismatch names the offending member rather than reporting a bare
    false
*/
bool
tests_read_write_mirrors_the_container_exactly()
{
    D_RW_CHECK(rw_check_mirror<std::vector<int> >("vector"));
    D_RW_CHECK(rw_check_mirror<std::list<int> >("list"));
    D_RW_CHECK(rw_check_mirror<std::deque<int> >("deque"));
    D_RW_CHECK(rw_check_mirror<std::array<int, 4> >("array"));
    D_RW_CHECK(rw_check_mirror<std::string>("string"));

    // other element types mirror too
    D_RW_CHECK(rw_check_mirror<std::vector<char> >("vector<char>"));
    D_RW_CHECK(rw_check_mirror<std::list<rw_probe_type> >("list<probe>"));

    // the rows really do differ between containers, so the mirror is not
    // trivially satisfied by every row being identical
    D_RW_CHECK(!rw_rows_match(rw_probe<std::vector<int> >(),
                              rw_probe<std::list<int> >()));
    D_RW_CHECK(!rw_rows_match(rw_probe<std::vector<int> >(),
                              rw_probe<std::array<int, 4> >()));
    D_RW_CHECK(!rw_rows_match(rw_probe<std::list<int> >(),
                              rw_probe<std::deque<int> >()));

    return true;
}

/*
tests_read_write_array_gains_no_mutator
  a fixed-size container.
  Tests the following:
  - wrapping a container with no growth operations does not conjure them
  - the observing surface still comes through in full
  - the non-const element accessors are still available, since a fixed-size
    container's elements are writable even though its size is not
*/
bool
tests_read_write_array_gains_no_mutator()
{
    typedef dj::read_write_container<std::array<int, 3> > handle;

    // no growth operations
    D_RW_CHECK(!rw_has_push_back<handle>::value);
    D_RW_CHECK(!rw_has_push_front<handle>::value);
    D_RW_CHECK(!rw_has_emplace_back<handle>::value);
    D_RW_CHECK(!rw_has_pop_back<handle>::value);
    D_RW_CHECK(!rw_has_pop_front<handle>::value);
    D_RW_CHECK(!rw_has_clear<handle>::value);
    D_RW_CHECK(!rw_has_resize<handle>::value);
    D_RW_CHECK(!rw_has_reserve<handle>::value);
    D_RW_CHECK(!rw_has_insert<handle>::value);
    D_RW_CHECK(!rw_has_erase<handle>::value);

    // because the container has none either
    D_RW_CHECK(!rw_has_push_back<std::array<int, 3> >::value);
    D_RW_CHECK(!rw_has_clear<std::array<int, 3> >::value);

    // the observing surface is complete
    D_RW_CHECK(rw_has_size<handle>::value);
    D_RW_CHECK(rw_has_empty<handle>::value);
    D_RW_CHECK(rw_has_sub<handle>::value);
    D_RW_CHECK(rw_has_front<handle>::value);
    D_RW_CHECK(rw_has_back<handle>::value);
    D_RW_CHECK(rw_has_data<handle>::value);
    D_RW_CHECK(rw_has_begin<handle>::value);
    D_RW_CHECK(rw_has_cbegin<handle>::value);

    // and the elements are writable, even though the size is fixed
    dj::read_write_container<std::array<int, 3> > a;

    a[0] = 1;
    a.front() = 2;
    a.back() = 3;
    *a.data() = 4;

    D_RW_CHECK(a[0] == 4);
    D_RW_CHECK(a[2] == 3);
    D_RW_CHECK(a.size() == 3u);

    return true;
}

/*
tests_read_write_list_keeps_both_ends
  a node container.
  Tests the following:
  - the front-end operations a list offers survive the wrapping
  - the members a list lacks are still absent from the handle
  - the handle behaves correctly at both ends at run time
*/
bool
tests_read_write_list_keeps_both_ends()
{
    typedef dj::read_write_container<std::list<int> > handle;

    // both ends are present
    D_RW_CHECK(rw_has_push_back<handle>::value);
    D_RW_CHECK(rw_has_push_front<handle>::value);
    D_RW_CHECK(rw_has_pop_back<handle>::value);
    D_RW_CHECK(rw_has_pop_front<handle>::value);
    D_RW_CHECK(rw_has_front<handle>::value);
    D_RW_CHECK(rw_has_back<handle>::value);

    // what a list lacks, the handle lacks
    D_RW_CHECK(!rw_has_sub<handle>::value);
    D_RW_CHECK(!rw_has_data<handle>::value);
    D_RW_CHECK(!rw_has_reserve<handle>::value);

    // and at run time both ends work
    handle l;

    l.push_back(2);
    l.push_front(1);
    l.push_back(3);

    D_RW_CHECK(l.size() == 3u);
    D_RW_CHECK(l.front() == 1);
    D_RW_CHECK(l.back() == 3);

    l.pop_front();
    D_RW_CHECK(l.front() == 2);

    l.pop_back();
    D_RW_CHECK(l.size() == 1u);
    D_RW_CHECK(l.front() == 2);
    D_RW_CHECK(l.back() == 2);

    return true;
}

/*
tests_read_write_deque_and_string_surfaces
  the two containers with unusual surface combinations.
  Tests the following:
  - a deque keeps random access and both ends but has no contiguous
    storage, so the handle offers subscript without data
  - a string keeps the full contiguous surface including data and reserve
  - both mirror their containers exactly
*/
bool
tests_read_write_deque_and_string_surfaces()
{
    typedef dj::read_write_container<std::deque<int> > deque_handle;
    typedef dj::read_write_container<std::string>      string_handle;

    // a deque: subscript without data
    D_RW_CHECK(rw_has_sub<deque_handle>::value);
    D_RW_CHECK(!rw_has_data<deque_handle>::value);
    D_RW_CHECK(rw_has_push_front<deque_handle>::value);
    D_RW_CHECK(!rw_has_reserve<deque_handle>::value);

    // a string: the full contiguous surface
    D_RW_CHECK(rw_has_sub<string_handle>::value);
    D_RW_CHECK(rw_has_data<string_handle>::value);
    D_RW_CHECK(rw_has_reserve<string_handle>::value);
    D_RW_CHECK(rw_has_push_back<string_handle>::value);
    D_RW_CHECK(!rw_has_push_front<string_handle>::value);

    // both mirror
    D_RW_CHECK(rw_check_mirror<std::deque<int> >("deque"));
    D_RW_CHECK(rw_check_mirror<std::string>("string"));

    // and both behave at run time
    deque_handle d;

    d.push_back(2);
    d.push_front(1);
    D_RW_CHECK(d[0] == 1);
    D_RW_CHECK(d[1] == 2);

    string_handle s;

    s.push_back('h');
    s.push_back('i');
    D_RW_CHECK(s.size() == 2u);
    D_RW_CHECK(s.data()[0] == 'h');
    s[1] = 'o';
    D_RW_CHECK(s[1] == 'o');

    return true;
}

/*
tests_read_write_is_the_unrestricted_sibling
  the trio, side by side.
  Tests the following:
  - against the same held container, read_write keeps every member that
    read_only seals away and every member that write_only seals away
  - the three overlays therefore partition one surface three ways
  - read_write's row is the union: nothing the container has is missing
*/
bool
tests_read_write_is_the_unrestricted_sibling()
{
    typedef std::vector<int> held;

    const rw_caps raw = rw_probe<held>();
    const rw_caps rw  = rw_probe<dj::read_write_container<held> >();
    const rw_caps ro  = rw_probe<dj::read_only_container<held> >();
    const rw_caps wo  = rw_probe<dj::write_only_container<held> >();

    // read_write is the only one that mirrors the container
    D_RW_CHECK(rw_rows_match(rw, raw));
    D_RW_CHECK(!rw_rows_match(ro, raw));
    D_RW_CHECK(!rw_rows_match(wo, raw));

    // read_only seals the mutating side; read_write keeps it
    D_RW_CHECK(!ro.push_back);
    D_RW_CHECK(!ro.clear);
    D_RW_CHECK(!ro.erase);
    D_RW_CHECK(rw.push_back);
    D_RW_CHECK(rw.clear);
    D_RW_CHECK(rw.erase);

    // write_only seals the reading side; read_write keeps it
    D_RW_CHECK(!wo.subscript);
    D_RW_CHECK(!wo.front);
    D_RW_CHECK(!wo.data);
    D_RW_CHECK(!wo.begin);
    D_RW_CHECK(rw.subscript);
    D_RW_CHECK(rw.front);
    D_RW_CHECK(rw.data);
    D_RW_CHECK(rw.begin);

    // all three agree on the value-free metadata
    D_RW_CHECK(ro.size && rw.size && wo.size);
    D_RW_CHECK(ro.empty && rw.empty && wo.empty);

    // and all three hold the identical container type
    D_RW_CHECK((std::is_same<
                    dj::read_write_container<held>::container_type,
                    dj::read_only_container<held>::container_type>::value));
    D_RW_CHECK((std::is_same<
                    dj::read_write_container<held>::container_type,
                    dj::write_only_container<held>::container_type>::value));

    return true;
}

/*
tests_read_write_const_surface_is_constexpr
  compile-time usability.
  Tests the following:
  - a handle over a literal-type container can be built at compile time and
    observed there through the const surface
  - size, empty, subscript and front all answer in a constant expression
  - the compile-time answers agree with the run-time ones
*/
bool
tests_read_write_const_surface_is_constexpr()
{
    typedef dj::read_write_container<std::array<int, 3> > handle;

    constexpr std::array<int, 3> raw = { { 4, 5, 6 } };
    constexpr handle             h(raw);

    static_assert(h.size() == 3u, "size in a constant expression");
    static_assert(!h.empty(),     "empty in a constant expression");
    static_assert(h[0] == 4,      "subscript in a constant expression");
    static_assert(h[2] == 6,      "subscript in a constant expression");
    static_assert(h.front() == 4, "front in a constant expression");
    static_assert(h.back() == 6,  "back in a constant expression");

    // the same values at run time
    D_RW_CHECK(h.size() == 3u);
    D_RW_CHECK(h[0] == 4);
    D_RW_CHECK(h.front() == 4);
    D_RW_CHECK(h.back() == 6);

    // the default constructor is constexpr too
    constexpr handle d;

    static_assert(d.size() == 3u, "default ctor is constexpr");
    D_RW_CHECK(d.size() == 3u);

    // a constexpr handle is const, so it offers only the const surface -
    // which is why the mutating forwarders' own constexpr-ness is governed
    // by the held container's lifetime rather than by this wrapper
    D_RW_CHECK(rw_const_result<decltype(h[0])>::value);

    return true;
}

/*
tests_read_write_detection_battery_is_sound
  the suite's own instrument.
  Tests the following:
  - the detectors report TRUE for members that genuinely exist on raw
    containers, so a mirror failure cannot be a false negative
  - they report FALSE for members those containers genuinely lack
  - the row comparison distinguishes rows that really do differ
*/
bool
tests_read_write_detection_battery_is_sound()
{
    // a vector really has these
    D_RW_CHECK(rw_has_push_back<std::vector<int> >::value);
    D_RW_CHECK(rw_has_emplace_back<std::vector<int> >::value);
    D_RW_CHECK(rw_has_pop_back<std::vector<int> >::value);
    D_RW_CHECK(rw_has_clear<std::vector<int> >::value);
    D_RW_CHECK(rw_has_resize<std::vector<int> >::value);
    D_RW_CHECK(rw_has_reserve<std::vector<int> >::value);
    D_RW_CHECK(rw_has_insert<std::vector<int> >::value);
    D_RW_CHECK(rw_has_erase<std::vector<int> >::value);
    D_RW_CHECK(rw_has_data<std::vector<int> >::value);

    // and really lacks these
    D_RW_CHECK(!rw_has_push_front<std::vector<int> >::value);
    D_RW_CHECK(!rw_has_pop_front<std::vector<int> >::value);

    // a list is the mirror image on those two
    D_RW_CHECK(rw_has_push_front<std::list<int> >::value);
    D_RW_CHECK(rw_has_pop_front<std::list<int> >::value);
    D_RW_CHECK(!rw_has_reserve<std::list<int> >::value);
    D_RW_CHECK(!rw_has_sub<std::list<int> >::value);
    D_RW_CHECK(!rw_has_data<std::list<int> >::value);

    // a deque has subscript but no data and no reserve
    D_RW_CHECK(rw_has_sub<std::deque<int> >::value);
    D_RW_CHECK(!rw_has_data<std::deque<int> >::value);
    D_RW_CHECK(!rw_has_reserve<std::deque<int> >::value);

    // the row comparison is not vacuously true
    D_RW_CHECK(rw_rows_match(rw_probe<std::vector<int> >(),
                             rw_probe<std::vector<int> >()));
    D_RW_CHECK(!rw_rows_match(rw_probe<std::vector<int> >(),
                              rw_probe<std::list<int> >()));

    // and a const handle drops exactly the mutating members
    typedef const dj::read_write_container<std::vector<int> > const_handle;

    D_RW_CHECK(rw_has_size<const_handle>::value);
    D_RW_CHECK(rw_has_cbegin<const_handle>::value);
    D_RW_CHECK(!rw_has_push_back<const_handle>::value);
    D_RW_CHECK(!rw_has_erase<const_handle>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
