/*******************************************************************************
* djinterp [cpp]                           read_write_container_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/container/read_write_container_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_write_exposes_container_type
  the held-container typedef.
  Tests the following:
  - container_type names the wrapped container exactly, unqualified
  - it is not a reference or a pointer to it
  - it tracks whatever container the handle was parameterised on
*/
bool tests_read_write_exposes_container_type()
{
    typedef dj::DReadWriteContainer<std::vector<int> > vec_handle;

    D_RW_CHECK((std::is_same<vec_handle::container_type,
                             std::vector<int> >::value));
    D_RW_CHECK(!std::is_reference<vec_handle::container_type>::value);
    D_RW_CHECK(!std::is_pointer<vec_handle::container_type>::value);
    D_RW_CHECK(!std::is_const<vec_handle::container_type>::value);

    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::list<char> >
                        ::container_type,
                    std::list<char> >::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::array<double, 2> >
                        ::container_type,
                    std::array<double, 2> >::value));

    // the three overlays over one container agree on the held type
    D_RW_CHECK((std::is_same<
                    vec_handle::container_type,
                    dj::DReadOnlyContainer<std::vector<int> >
                        ::container_type>::value));

    return true;
}

/*
tests_read_write_exposes_value_type
  the element typedef.
  Tests the following:
  - value_type is taken from the held container rather than restated
  - it follows the element type across several containers
  - it is the plain element type, not a reference to it
*/
bool tests_read_write_exposes_value_type()
{
    typedef dj::DReadWriteContainer<std::vector<int> > vec_handle;

    D_RW_CHECK((std::is_same<vec_handle::value_type, int>::value));
    D_RW_CHECK((std::is_same<vec_handle::value_type,
                             std::vector<int>::value_type>::value));
    D_RW_CHECK(!std::is_reference<vec_handle::value_type>::value);
    D_RW_CHECK(!std::is_const<vec_handle::value_type>::value);

    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::list<char> >::value_type,
                    char>::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::string>::value_type,
                    char>::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::array<double, 2> >
                        ::value_type,
                    double>::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::vector<rw_probe_type> >
                        ::value_type,
                    rw_probe_type>::value));

    return true;
}

/*
tests_read_write_capability_is_read_write
  the overlay's identity to generic code.
  Tests the following:
  - the capability typedef names the read_write tag
  - it is not either sibling tag, so a dispatcher can tell the three apart
  - the typedef is the same whatever container is wrapped
*/
bool tests_read_write_capability_is_read_write()
{
    typedef dj::DReadWriteContainer<std::vector<int> > handle;

    D_RW_CHECK((std::is_same<handle::capability, dj::DReadWrite>::value));
    D_RW_CHECK(!(std::is_same<handle::capability, dj::DReadOnly>::value));
    D_RW_CHECK(!(std::is_same<handle::capability, dj::DWriteOnly>::value));

    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::list<char> >::capability,
                    dj::DReadWrite>::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::array<int, 1> >::capability,
                    dj::DReadWrite>::value));
    D_RW_CHECK((std::is_same<
                    dj::DReadWriteContainer<std::string>::capability,
                    dj::DReadWrite>::value));

    // the three overlays over one container carry three different tags
    D_RW_CHECK(!(std::is_same<
                     handle::capability,
                     dj::DReadOnlyContainer<std::vector<int> >
                         ::capability>::value));
    D_RW_CHECK(!(std::is_same<
                     handle::capability,
                     dj::DWriteOnlyContainer<std::vector<int> >
                         ::capability>::value));

    return true;
}

/*
tests_read_write_capability_tag_semantics
  what the tag itself reports.
  Tests the following:
  - the tag advertises that both reading and writing are permitted
  - it is the only one of the three that permits both
  - its answers are compile-time constants, usable in a static assertion
*/
bool tests_read_write_capability_tag_semantics()
{
    typedef dj::DReadWriteContainer<std::vector<int> >::capability cap;

    D_RW_CHECK(cap::can_read);
    D_RW_CHECK(cap::can_write);

    static_assert(cap::can_read,  "read_write must be readable");
    static_assert(cap::can_write, "read_write must be writable");

    D_RW_CHECK(std::string(cap::name()) == "read_write");

    // it is the only tag of the three permitting both
    D_RW_CHECK(cap::can_read && cap::can_write);
    D_RW_CHECK(!(dj::DReadOnly::can_read && dj::DReadOnly::can_write));
    D_RW_CHECK(!(dj::DWriteOnly::can_read && dj::DWriteOnly::can_write));

    // and the three names are distinct
    D_RW_CHECK(std::string(dj::read_only::name()) != std::string(cap::name()));
    D_RW_CHECK(std::string(dj::write_only::name()) !=
               std::string(cap::name()));

    return true;
}

/*
tests_read_write_typedefs_track_container
  the three typedefs together.
  Tests the following:
  - container_type and value_type stay consistent with one another
  - the capability stays fixed while the other two vary
  - the relationship holds across sequence, node and fixed-size containers
*/
bool tests_read_write_typedefs_track_container()
{
    typedef dj::DReadWriteContainer<std::vector<int> >      h_vec;
    typedef dj::DReadWriteContainer<std::list<char> >       h_list;
    typedef dj::DReadWriteContainer<std::array<double, 4> > h_arr;
    typedef dj::DReadWriteContainer<std::deque<long> >      h_deq;

    D_RW_CHECK((std::is_same<h_vec::value_type,
                             h_vec::container_type::value_type>::value));
    D_RW_CHECK((std::is_same<h_list::value_type,
                             h_list::container_type::value_type>::value));
    D_RW_CHECK((std::is_same<h_arr::value_type,
                             h_arr::container_type::value_type>::value));
    D_RW_CHECK((std::is_same<h_deq::value_type,
                             h_deq::container_type::value_type>::value));

    D_RW_CHECK((std::is_same<h_vec::capability, dj::DReadWrite>::value));
    D_RW_CHECK((std::is_same<h_list::capability, dj::DReadWrite>::value));
    D_RW_CHECK((std::is_same<h_arr::capability, dj::DReadWrite>::value));
    D_RW_CHECK((std::is_same<h_deq::capability, dj::DReadWrite>::value));

    D_RW_CHECK(!(std::is_same<h_vec::value_type,
                              h_list::value_type>::value));
    D_RW_CHECK(!(std::is_same<h_vec::container_type,
                              h_deq::container_type>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
