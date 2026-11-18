/*******************************************************************************
* djinterp [cpp]                           write_only_container_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/container/write_only_container_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "write_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_write_only_exposes_container_type
  the held-container typedef.
  Tests the following:
  - container_type names the wrapped container exactly, unqualified
  - it is not a reference or a pointer to it
  - it tracks whatever container the sink was parameterised on
*/
bool tests_write_only_exposes_container_type()
{
    typedef dj::DWriteOnlyContainer<std::vector<int> > vec_sink;

    D_WO_CHECK((std::is_same<vec_sink::container_type,
                             std::vector<int> >::value));
    D_WO_CHECK(!std::is_reference<vec_sink::container_type>::value);
    D_WO_CHECK(!std::is_pointer<vec_sink::container_type>::value);
    D_WO_CHECK(!std::is_const<vec_sink::container_type>::value);

    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::list<char> >
                        ::container_type,
                    std::list<char> >::value));
    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::string>::container_type,
                    std::string>::value));

    // the three overlays over one container agree on the held type
    D_WO_CHECK((std::is_same<
                    vec_sink::container_type,
                    dj::DReadWriteContainer<std::vector<int> >
                        ::container_type>::value));

    return true;
}

/*
tests_write_only_exposes_value_type
  the element typedef.
  Tests the following:
  - value_type is taken from the held container, even though no member
    returns a value of it
  - it follows the element type across several containers
  - it is the plain element type, not a reference to it
*/
bool tests_write_only_exposes_value_type()
{
    typedef dj::DWriteOnlyContainer<std::vector<int> > vec_sink;

    D_WO_CHECK((std::is_same<vec_sink::value_type, int>::value));
    D_WO_CHECK((std::is_same<vec_sink::value_type,
                             std::vector<int>::value_type>::value));
    D_WO_CHECK(!std::is_reference<vec_sink::value_type>::value);
    D_WO_CHECK(!std::is_const<vec_sink::value_type>::value);

    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::list<char> >::value_type,
                    char>::value));
    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::string>::value_type,
                    char>::value));
    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::vector<wo_probe_type> >
                        ::value_type,
                    wo_probe_type>::value));

    // the typedef is what a caller needs in order to PUT a value in, which
    // is why it survives even though nothing hands one back
    dj::DWriteOnlyContainer<std::vector<int> > sink;
    vec_sink::value_type                        v = 5;

    sink.push_back(v);
    D_WO_CHECK(sink.size() == 1u);

    return true;
}

/*
tests_write_only_capability_is_write_only
  the overlay's identity to generic code.
  Tests the following:
  - the capability typedef names the write_only tag
  - it is not either sibling tag, so a dispatcher can tell the three apart
  - the typedef is the same whatever container is wrapped
*/
bool tests_write_only_capability_is_write_only()
{
    typedef dj::DWriteOnlyContainer<std::vector<int> > sink;

    D_WO_CHECK((std::is_same<sink::capability, dj::DWriteOnly>::value));
    D_WO_CHECK(!(std::is_same<sink::capability, dj::DReadOnly>::value));
    D_WO_CHECK(!(std::is_same<sink::capability, dj::DReadWrite>::value));

    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::list<char> >::capability,
                    dj::DWriteOnly>::value));
    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::string>::capability,
                    dj::DWriteOnly>::value));
    D_WO_CHECK((std::is_same<
                    dj::DWriteOnlyContainer<std::array<int, 1> >::capability,
                    dj::DWriteOnly>::value));

    // the three overlays over one container carry three different tags
    D_WO_CHECK(!(std::is_same<
                     sink::capability,
                     dj::DReadOnlyContainer<std::vector<int> >
                         ::capability>::value));
    D_WO_CHECK(!(std::is_same<
                     sink::capability,
                     dj::DReadWriteContainer<std::vector<int> >
                         ::capability>::value));

    return true;
}

/*
tests_write_only_capability_tag_semantics
  what the tag itself reports.
  Tests the following:
  - the tag advertises that writing is permitted and reading is not
  - it is the only one of the three that permits writing without reading
  - its answers are compile-time constants, usable in a static assertion
*/
bool tests_write_only_capability_tag_semantics()
{
    typedef dj::DWriteOnlyContainer<std::vector<int> >::capability cap;

    D_WO_CHECK(!cap::can_read);
    D_WO_CHECK(cap::can_write);

    static_assert(!cap::can_read, "write_only must not be readable");
    static_assert(cap::can_write, "write_only must be writable");

    D_WO_CHECK(std::string(cap::name()) == "write_only");

    // it is the only tag that writes without reading
    D_WO_CHECK(cap::can_write && !cap::can_read);
    D_WO_CHECK(!(dj::DReadOnly::can_write && !dj::DReadOnly::can_read));
    D_WO_CHECK(!(dj::DReadWrite::can_write && !dj::DReadWrite::can_read));

    // the three tags cover three distinct combinations
    D_WO_CHECK(dj::DReadOnly::can_read && !dj::DReadOnly::can_write);
    D_WO_CHECK(dj::DReadWrite::can_read && dj::DReadWrite::can_write);
    D_WO_CHECK(!dj::DWriteOnly::can_read && dj::DWriteOnly::can_write);

    // and the three names are distinct
    D_WO_CHECK(std::string(dj::read_only::name()) != std::string(cap::name()));
    D_WO_CHECK(std::string(dj::read_write::name()) !=
               std::string(cap::name()));

    return true;
}

/*
tests_write_only_typedefs_track_container
  the three typedefs together.
  Tests the following:
  - container_type and value_type stay consistent with one another
  - the capability stays fixed while the other two vary
  - the relationship holds across sequence, node and character containers
*/
bool tests_write_only_typedefs_track_container()
{
    typedef dj::DWriteOnlyContainer<std::vector<int> >  s_vec;
    typedef dj::DWriteOnlyContainer<std::list<char> >   s_list;
    typedef dj::DWriteOnlyContainer<std::string>        s_str;
    typedef dj::DWriteOnlyContainer<std::deque<long> >  s_deq;

    D_WO_CHECK((std::is_same<s_vec::value_type,
                             s_vec::container_type::value_type>::value));
    D_WO_CHECK((std::is_same<s_list::value_type,
                             s_list::container_type::value_type>::value));
    D_WO_CHECK((std::is_same<s_str::value_type,
                             s_str::container_type::value_type>::value));
    D_WO_CHECK((std::is_same<s_deq::value_type,
                             s_deq::container_type::value_type>::value));

    D_WO_CHECK((std::is_same<s_vec::capability, dj::DWriteOnly>::value));
    D_WO_CHECK((std::is_same<s_list::capability, dj::DWriteOnly>::value));
    D_WO_CHECK((std::is_same<s_str::capability, dj::DWriteOnly>::value));
    D_WO_CHECK((std::is_same<s_deq::capability, dj::DWriteOnly>::value));

    D_WO_CHECK(!(std::is_same<s_vec::value_type, s_list::value_type>::value));
    D_WO_CHECK(!(std::is_same<s_vec::container_type,
                              s_deq::container_type>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
