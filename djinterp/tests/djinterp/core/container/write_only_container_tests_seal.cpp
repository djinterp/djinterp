/*******************************************************************************
* djinterp [core]                            write_only_container_tests_seal.cpp
*
*
* path:      /tests/djinterp/core/container/write_only_container_tests_seal.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "write_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_write_only_exposes_no_reader
  the central negative claim.
  Tests the following:
  - not one of the eight probed reading members is present on the sink
  - the roll-up predicate agrees, so a reading member added later without a
    matching detector still trips this body
  - the metadata and append members ARE present, so the absence is
    selective rather than the detectors failing on everything
*/
bool
tests_write_only_exposes_no_reader()
{
    typedef dj::write_only_container<std::vector<int> > sink;

    const wo_caps c = wo_probe<sink>();

    // every reading member, named individually
    D_WO_CHECK(!c.subscript);
    D_WO_CHECK(!c.front);
    D_WO_CHECK(!c.back);
    D_WO_CHECK(!c.data);
    D_WO_CHECK(!c.begin);
    D_WO_CHECK(!c.end);
    D_WO_CHECK(!c.cbegin);
    D_WO_CHECK(!c.cend);

    // and the roll-up
    D_WO_CHECK(!wo_any_reader(c));

    // the detectors are not simply failing on everything
    D_WO_CHECK(c.size);
    D_WO_CHECK(c.empty);
    D_WO_CHECK(c.push_back);
    D_WO_CHECK(c.emplace_back);
    D_WO_CHECK(c.clear);

    // the very same detectors find those readers on the held container
    const wo_caps raw = wo_probe<std::vector<int> >();

    D_WO_CHECK(raw.subscript);
    D_WO_CHECK(raw.front);
    D_WO_CHECK(raw.back);
    D_WO_CHECK(raw.data);
    D_WO_CHECK(raw.begin);
    D_WO_CHECK(raw.cbegin);
    D_WO_CHECK(wo_any_reader(raw));

    return true;
}

/*
tests_write_only_seal_holds_for_every_container
  the absence, across container types.
  Tests the following:
  - no reading member appears whatever container is wrapped
  - this includes containers offering the full contiguous reading surface,
    so the seal is a property of the wrapper rather than of what it holds
  - it holds for the fixed-size case too, whose sink is entirely inert
*/
bool
tests_write_only_seal_holds_for_every_container()
{
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<dj::write_only_container<std::vector<int> > >()));
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<dj::write_only_container<std::list<int> > >()));
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<dj::write_only_container<std::deque<int> > >()));
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<dj::write_only_container<std::string> >()));
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<
                       dj::write_only_container<std::array<int, 3> > >()));
    D_WO_CHECK(!wo_any_reader(
                   wo_probe<
                       dj::write_only_container<
                           std::vector<wo_probe_type> > >()));

    // the underlying containers really do offer readers, which is what
    // makes the assertions above meaningful
    D_WO_CHECK(wo_any_reader(wo_probe<std::vector<int> >()));
    D_WO_CHECK(wo_any_reader(wo_probe<std::list<int> >()));
    D_WO_CHECK(wo_any_reader(wo_probe<std::deque<int> >()));
    D_WO_CHECK(wo_any_reader(wo_probe<std::string>()));
    D_WO_CHECK(wo_any_reader(wo_probe<std::array<int, 3> >()));

    // even a const sink offers no reader, since there was none to begin
    // with - unlike read_only, where constness is what does the work
    typedef const dj::write_only_container<std::vector<int> > const_sink;

    D_WO_CHECK(!wo_any_reader(wo_probe<const_sink>()));
    D_WO_CHECK(wo_has_size<const_sink>::value);

    return true;
}

/*
tests_write_only_omits_positional_edits
  the edits that would need a read first.
  Tests the following:
  - insert, erase, pop_back, pop_front, resize and reserve are all absent,
    since each depends on a position within the container
  - this holds even for containers that offer every one of them
  - the three append members remain, so the omission is selective
*/
bool
tests_write_only_omits_positional_edits()
{
    typedef dj::write_only_container<std::vector<int> > vec_sink;
    typedef dj::write_only_container<std::list<int> >   list_sink;
    typedef dj::write_only_container<std::deque<int> >  deq_sink;

    // named individually
    D_WO_CHECK(!wo_has_insert<vec_sink>::value);
    D_WO_CHECK(!wo_has_erase<vec_sink>::value);
    D_WO_CHECK(!wo_has_pop_back<vec_sink>::value);
    D_WO_CHECK(!wo_has_pop_front<vec_sink>::value);
    D_WO_CHECK(!wo_has_resize<vec_sink>::value);
    D_WO_CHECK(!wo_has_reserve<vec_sink>::value);
    D_WO_CHECK(!wo_has_push_front<vec_sink>::value);

    // the roll-up, across containers
    D_WO_CHECK(!wo_any_positional_edit(wo_probe<vec_sink>()));
    D_WO_CHECK(!wo_any_positional_edit(wo_probe<list_sink>()));
    D_WO_CHECK(!wo_any_positional_edit(wo_probe<deq_sink>()));
    D_WO_CHECK(!wo_any_positional_edit(
                   wo_probe<dj::write_only_container<std::string> >()));

    // a list offers push_front, pop_front, pop_back, insert, erase and
    // resize; not one survives the wrapping
    D_WO_CHECK(wo_any_positional_edit(wo_probe<std::list<int> >()));
    D_WO_CHECK(wo_has_push_front<std::list<int> >::value);
    D_WO_CHECK(wo_has_pop_front<std::list<int> >::value);
    D_WO_CHECK(wo_has_erase<std::list<int> >::value);
    D_WO_CHECK(!wo_has_push_front<list_sink>::value);
    D_WO_CHECK(!wo_has_pop_front<list_sink>::value);
    D_WO_CHECK(!wo_has_erase<list_sink>::value);

    // while the three append members remain
    const wo_caps c = wo_probe<vec_sink>();

    D_WO_CHECK(c.push_back);
    D_WO_CHECK(c.emplace_back);
    D_WO_CHECK(c.clear);

    return true;
}

/*
tests_write_only_never_yields_the_container
  the escape-hatch check.
  Tests the following:
  - no member hands back the held container itself, by value, reference or
    pointer - which would return the whole reading surface at a stroke
  - the sink is not convertible to the container it holds
  - container_type names the held type but is only a type
*/
bool
tests_write_only_never_yields_the_container()
{
    typedef dj::write_only_container<std::vector<int> > sink;
    typedef std::vector<int>                            held;

    dj::write_only_container<std::vector<int> > s;

    // the two observers return neither the container nor a handle to it
    D_WO_CHECK(!(std::is_same<decltype(s.size()), held>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), held&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), const held&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), held*>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.empty()), held>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.empty()), const held&>::value));

    // and the sink is not convertible to the container
    D_WO_CHECK(!(std::is_convertible<sink, held>::value));
    D_WO_CHECK(!(std::is_convertible<sink, held&>::value));
    D_WO_CHECK(!(std::is_convertible<sink, const held&>::value));
    D_WO_CHECK(!(std::is_convertible<sink, held*>::value));

    // nor is it constructible the other way round in a way that would let
    // a caller round-trip the contents out
    D_WO_CHECK(!(std::is_constructible<held, sink>::value));

    // container_type names the held type but no member returns one
    D_WO_CHECK((std::is_same<sink::container_type, held>::value));

    // the same holds for the other containers
    typedef dj::write_only_container<std::list<int> > list_sink;

    D_WO_CHECK(!(std::is_convertible<list_sink, std::list<int> >::value));
    D_WO_CHECK(!(std::is_constructible<std::list<int>, list_sink>::value));

    return true;
}

/*
tests_write_only_no_member_returns_an_element
  the value-level statement of the seal.
  Tests the following:
  - no member's return type is the element type, nor a reference or pointer
    to it, so no value can leave the sink by any route
  - this is checked for both surviving observers and for the append members
  - the append members take an element in but hand nothing back
*/
bool
tests_write_only_no_member_returns_an_element()
{
    typedef dj::write_only_container<std::vector<int> > sink;
    typedef sink::value_type                            element;

    dj::write_only_container<std::vector<int> > s;

    // the observers
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), const element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), element*>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.size()), const element*>::value));

    D_WO_CHECK(!(std::is_same<decltype(s.empty()), element>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.empty()), element&>::value));

    // the append members: push_back and clear both return void here, and
    // in no case an element
    D_WO_CHECK((std::is_same<decltype(s.push_back(0)), void>::value));
    D_WO_CHECK((std::is_same<decltype(s.clear()), void>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.push_back(0)), element>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.push_back(0)), element&>::value));
    D_WO_CHECK(!(std::is_same<decltype(s.clear()), element&>::value));

    // and the element type is a plain type, so the seal is not carried by
    // qualification the way read_only's is
    D_WO_CHECK(!std::is_const<element>::value);
    D_WO_CHECK(!std::is_reference<element>::value);

    // there is simply no accessor to qualify: the reading surface is gone
    D_WO_CHECK(!wo_any_reader(wo_probe<sink>()));

    return true;
}

/*
tests_write_only_is_stricter_than_read_write
  the trio, side by side.
  Tests the following:
  - against the same held container the sink keeps strictly less than the
    unrestricted sibling does
  - every member the sink has, read_write has too
  - read_write has members the sink does not, so the containment is proper
*/
bool
tests_write_only_is_stricter_than_read_write()
{
    typedef std::vector<int> held;

    const wo_caps raw = wo_probe<held>();
    const wo_caps wo  = wo_probe<dj::write_only_container<held> >();
    const wo_caps rw  = wo_probe<dj::read_write_container<held> >();

    // read_write mirrors the container; write_only does not
    D_WO_CHECK(rw.subscript == raw.subscript);
    D_WO_CHECK(rw.data == raw.data);
    D_WO_CHECK(rw.erase == raw.erase);
    D_WO_CHECK(wo.subscript != raw.subscript);
    D_WO_CHECK(wo.data != raw.data);

    // everything the sink has, read_write has too
    D_WO_CHECK(!wo.size || rw.size);
    D_WO_CHECK(!wo.empty || rw.empty);
    D_WO_CHECK(!wo.push_back || rw.push_back);
    D_WO_CHECK(!wo.emplace_back || rw.emplace_back);
    D_WO_CHECK(!wo.clear || rw.clear);

    // and read_write has members the sink lacks, so it is proper
    D_WO_CHECK(rw.subscript && !wo.subscript);
    D_WO_CHECK(rw.front && !wo.front);
    D_WO_CHECK(rw.data && !wo.data);
    D_WO_CHECK(rw.begin && !wo.begin);
    D_WO_CHECK(rw.erase && !wo.erase);
    D_WO_CHECK(rw.pop_back && !wo.pop_back);

    // both hold the identical container type
    D_WO_CHECK((std::is_same<
                    dj::write_only_container<held>::container_type,
                    dj::read_write_container<held>::container_type>::value));

    return true;
}

/*
tests_write_only_is_disjoint_from_read_only
  the two restrictions, opposed.
  Tests the following:
  - the two restricted overlays seal opposite halves of the surface
  - read_only keeps the readers and drops the writers; write_only does the
    reverse
  - they share only the value-free metadata, which is why that pair is
    safe for both
*/
bool
tests_write_only_is_disjoint_from_read_only()
{
    typedef std::vector<int> held;

    const wo_caps wo = wo_probe<dj::write_only_container<held> >();
    const wo_caps ro = wo_probe<dj::read_only_container<held> >();

    // read_only keeps the readers, write_only drops them
    D_WO_CHECK(ro.subscript);
    D_WO_CHECK(ro.front);
    D_WO_CHECK(ro.data);
    D_WO_CHECK(ro.begin);
    D_WO_CHECK(!wo.subscript);
    D_WO_CHECK(!wo.front);
    D_WO_CHECK(!wo.data);
    D_WO_CHECK(!wo.begin);

    // write_only keeps the appends, read_only drops them
    D_WO_CHECK(wo.push_back);
    D_WO_CHECK(wo.emplace_back);
    D_WO_CHECK(wo.clear);
    D_WO_CHECK(!ro.push_back);
    D_WO_CHECK(!ro.emplace_back);
    D_WO_CHECK(!ro.clear);

    // neither keeps a positional edit
    D_WO_CHECK(!wo_any_positional_edit(wo));
    D_WO_CHECK(!wo_any_positional_edit(ro));

    // they share exactly the value-free metadata
    D_WO_CHECK(wo.size && ro.size);
    D_WO_CHECK(wo.empty && ro.empty);

    // and the two capability tags are opposed on both axes
    D_WO_CHECK(dj::read_only::can_read != dj::write_only::can_read);
    D_WO_CHECK(dj::read_only::can_write != dj::write_only::can_write);

    return true;
}

/*
tests_write_only_detection_battery_is_sound
  the suite's own instrument.
  Tests the following:
  - the detectors report TRUE for members that genuinely exist on raw
    containers, so a false negative cannot silently mask a leaked reader
  - they report FALSE for members those containers genuinely lack
  - the two roll-up helpers agree with the individual detectors
*/
bool
tests_write_only_detection_battery_is_sound()
{
    // a vector really has these readers
    D_WO_CHECK(wo_has_sub<std::vector<int> >::value);
    D_WO_CHECK(wo_has_front<std::vector<int> >::value);
    D_WO_CHECK(wo_has_back<std::vector<int> >::value);
    D_WO_CHECK(wo_has_data<std::vector<int> >::value);
    D_WO_CHECK(wo_has_begin<std::vector<int> >::value);
    D_WO_CHECK(wo_has_end<std::vector<int> >::value);
    D_WO_CHECK(wo_has_cbegin<std::vector<int> >::value);
    D_WO_CHECK(wo_has_cend<std::vector<int> >::value);

    // and these edits
    D_WO_CHECK(wo_has_insert<std::vector<int> >::value);
    D_WO_CHECK(wo_has_erase<std::vector<int> >::value);
    D_WO_CHECK(wo_has_resize<std::vector<int> >::value);
    D_WO_CHECK(wo_has_reserve<std::vector<int> >::value);

    // and really lacks these
    D_WO_CHECK(!wo_has_push_front<std::vector<int> >::value);
    D_WO_CHECK(!wo_has_pop_front<std::vector<int> >::value);

    // a list is the mirror image on those two, and has no subscript
    D_WO_CHECK(wo_has_push_front<std::list<int> >::value);
    D_WO_CHECK(wo_has_pop_front<std::list<int> >::value);
    D_WO_CHECK(!wo_has_sub<std::list<int> >::value);
    D_WO_CHECK(!wo_has_data<std::list<int> >::value);

    // the roll-ups agree with the individual detectors
    const wo_caps raw = wo_probe<std::vector<int> >();

    D_WO_CHECK(wo_any_reader(raw) ==
               (raw.subscript || raw.front  || raw.back   || raw.data ||
                raw.begin     || raw.end    || raw.cbegin || raw.cend));
    D_WO_CHECK(wo_any_positional_edit(raw) ==
               (raw.push_front || raw.pop_back || raw.pop_front ||
                raw.resize     || raw.reserve  || raw.insert    ||
                raw.erase));

    // an array has readers but essentially no edits
    const wo_caps arr = wo_probe<std::array<int, 3> >();

    D_WO_CHECK(wo_any_reader(arr));
    D_WO_CHECK(!wo_any_positional_edit(arr));
    D_WO_CHECK(!arr.push_back);
    D_WO_CHECK(!arr.clear);

    return true;
}

NS_END  // testing
NS_END  // djinterp
