/*******************************************************************************
* djinterp [core]                           read_only_container_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_only_exposes_container_type
  the held-container typedef.
  Tests the following:
  - container_type names the wrapped container exactly, unqualified
  - it is not a reference or a pointer to it
  - it tracks whatever container the handle was parameterised on
*/
bool
tests_read_only_exposes_container_type()
{
    typedef dj::read_only_container<std::vector<int> > vec_handle;

    D_RO_CHECK((std::is_same<vec_handle::container_type,
                             std::vector<int> >::value));

    // not a reference, not a pointer, not const-qualified
    D_RO_CHECK(!std::is_reference<vec_handle::container_type>::value);
    D_RO_CHECK(!std::is_pointer<vec_handle::container_type>::value);
    D_RO_CHECK(!std::is_const<vec_handle::container_type>::value);

    // it follows the parameter
    typedef dj::read_only_container<std::list<char> > list_handle;

    D_RO_CHECK((std::is_same<list_handle::container_type,
                             std::list<char> >::value));

    typedef dj::read_only_container<std::array<double, 2> > arr_handle;

    D_RO_CHECK((std::is_same<arr_handle::container_type,
                             std::array<double, 2> >::value));

    return true;
}

/*
tests_read_only_exposes_value_type
  the element typedef.
  Tests the following:
  - value_type is taken from the held container rather than restated
  - it follows the element type across several containers
  - it is the plain element type, not a reference to it
*/
bool
tests_read_only_exposes_value_type()
{
    typedef dj::read_only_container<std::vector<int> > vec_handle;

    D_RO_CHECK((std::is_same<vec_handle::value_type, int>::value));
    D_RO_CHECK((std::is_same<vec_handle::value_type,
                             std::vector<int>::value_type>::value));
    D_RO_CHECK(!std::is_reference<vec_handle::value_type>::value);

    // other element types
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<std::list<char> >::value_type,
                    char>::value));
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<std::string>::value_type,
                    char>::value));
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<
                        std::array<double, 2> >::value_type,
                    double>::value));

    // including a user-defined element type
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<
                        std::vector<ro_probe_type> >::value_type,
                    ro_probe_type>::value));

    return true;
}

/*
tests_read_only_capability_is_read_only
  the overlay's identity to generic code.
  Tests the following:
  - the capability typedef names the read_only tag
  - it is not either sibling tag, so a dispatcher can tell the three apart
  - the typedef is the same whatever container is wrapped
*/
bool
tests_read_only_capability_is_read_only()
{
    typedef dj::read_only_container<std::vector<int> > handle;

    D_RO_CHECK((std::is_same<handle::capability, dj::read_only>::value));

    // and not a sibling
    D_RO_CHECK(!(std::is_same<handle::capability, dj::read_write>::value));
    D_RO_CHECK(!(std::is_same<handle::capability, dj::write_only>::value));

    // the tag does not vary with the container
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<std::list<char> >::capability,
                    dj::read_only>::value));
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<std::array<int, 1> >::capability,
                    dj::read_only>::value));
    D_RO_CHECK((std::is_same<
                    dj::read_only_container<std::string>::capability,
                    dj::read_only>::value));

    return true;
}

/*
tests_read_only_capability_tag_semantics
  what the tag itself reports.
  Tests the following:
  - the tag advertises that reading is permitted and writing is not
  - it names itself for diagnostics
  - its answers are compile-time constants, usable in a static assertion
*/
bool
tests_read_only_capability_tag_semantics()
{
    typedef dj::read_only_container<std::vector<int> >::capability cap;

    D_RO_CHECK(cap::can_read);
    D_RO_CHECK(!cap::can_write);

    // the values really are constant expressions
    static_assert(cap::can_read, "read_only must be readable");
    static_assert(!cap::can_write, "read_only must not be writable");

    // it names itself
    D_RO_CHECK(std::string(cap::name()) == "read_only");

    // the sibling tags differ, which is what makes the axis meaningful
    D_RO_CHECK(dj::read_write::can_read);
    D_RO_CHECK(dj::read_write::can_write);
    D_RO_CHECK(!dj::write_only::can_read);
    D_RO_CHECK(dj::write_only::can_write);

    D_RO_CHECK(std::string(dj::read_write::name()) == "read_write");
    D_RO_CHECK(std::string(dj::write_only::name()) == "write_only");

    // read_only is the only tag that reads but does not write
    D_RO_CHECK(cap::can_read && !cap::can_write);

    return true;
}

/*
tests_read_only_typedefs_track_container
  the three typedefs together.
  Tests the following:
  - container_type and value_type stay consistent with one another
  - the capability stays fixed while the other two vary
  - the relationship holds across sequence, node and fixed-size containers
*/
bool
tests_read_only_typedefs_track_container()
{
    typedef dj::read_only_container<std::vector<int> >      h_vec;
    typedef dj::read_only_container<std::list<char> >       h_list;
    typedef dj::read_only_container<std::array<double, 4> > h_arr;
    typedef dj::read_only_container<std::deque<long> >      h_deq;

    // value_type is always the container's own
    D_RO_CHECK((std::is_same<h_vec::value_type,
                             h_vec::container_type::value_type>::value));
    D_RO_CHECK((std::is_same<h_list::value_type,
                             h_list::container_type::value_type>::value));
    D_RO_CHECK((std::is_same<h_arr::value_type,
                             h_arr::container_type::value_type>::value));
    D_RO_CHECK((std::is_same<h_deq::value_type,
                             h_deq::container_type::value_type>::value));

    // the capability is invariant across all four
    D_RO_CHECK((std::is_same<h_vec::capability, dj::read_only>::value));
    D_RO_CHECK((std::is_same<h_list::capability, dj::read_only>::value));
    D_RO_CHECK((std::is_same<h_arr::capability, dj::read_only>::value));
    D_RO_CHECK((std::is_same<h_deq::capability, dj::read_only>::value));

    // while the other two are not
    D_RO_CHECK(!(std::is_same<h_vec::value_type,
                              h_list::value_type>::value));
    D_RO_CHECK(!(std::is_same<h_vec::container_type,
                              h_deq::container_type>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
