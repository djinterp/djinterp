/*******************************************************************************
* djinterp [test]                          write_only_container_tests_append.cpp
*
*
* path:      /tests/djinterp/test/write_only_container_tests_append.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "../core/container/write_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_write_only_push_back_appends
  the append operation.
  Tests the following:
  - push_back adds exactly one element per call
  - the reported count grows by one each time
  - an lvalue argument is accepted as readily as a temporary
  - it works on every container that offers it
*/
bool
tests_write_only_push_back_appends()
{
    dj::write_only_container<std::vector<int> > sink;

    sink.push_back(1);
    D_WO_CHECK(sink.size() == 1u);
    D_WO_CHECK(!sink.empty());

    sink.push_back(2);
    sink.push_back(3);
    D_WO_CHECK(sink.size() == 3u);

    // an lvalue argument
    int value = 4;

    sink.push_back(value);
    D_WO_CHECK(sink.size() == 4u);

    // a const lvalue
    const int cvalue = 5;

    sink.push_back(cvalue);
    D_WO_CHECK(sink.size() == 5u);

    // a node container
    dj::write_only_container<std::list<int> > l;

    l.push_back(7);
    l.push_back(8);
    D_WO_CHECK(l.size() == 2u);

    // a character container
    dj::write_only_container<std::string> s;

    s.push_back('a');
    s.push_back('b');
    D_WO_CHECK(s.size() == 2u);

    // a deque
    dj::write_only_container<std::deque<int> > d;

    d.push_back(1);
    D_WO_CHECK(d.size() == 1u);

    return true;
}

/*
tests_write_only_emplace_back_forwards_args
  in-place construction at the end.
  Tests the following:
  - emplace_back appends one element
  - several arguments reach the ELEMENT's constructor, not the container's
  - a call with no arguments constructs a default element
*/
bool
tests_write_only_emplace_back_forwards_args()
{
    dj::write_only_container<std::vector<int> > sink;

    sink.emplace_back(1);
    sink.emplace_back(2);
    D_WO_CHECK(sink.size() == 2u);

    // several arguments, forwarded to a two-argument element constructor.
    // The element counts its own instances, so the append is observable
    // without reading the element back.
    const int before = wo_probe_type::s_constructed;

    dj::write_only_container<std::vector<wo_probe_type> > objs;

    objs.emplace_back(7, 'q');
    D_WO_CHECK(objs.size() == 1u);
    D_WO_CHECK(wo_probe_type::s_constructed > before);

    objs.emplace_back(-3, 'z');
    D_WO_CHECK(objs.size() == 2u);

    // no arguments at all
    objs.emplace_back();
    D_WO_CHECK(objs.size() == 3u);

    // a node container
    dj::write_only_container<std::list<wo_probe_type> > l;

    l.emplace_back(5, 'm');
    D_WO_CHECK(l.size() == 1u);

    // and the sink still reports correctly after many emplacements
    std::size_t i = 0;

    for (i = 0; i < 20u; ++i)
    {
        sink.emplace_back(static_cast<int>(i));
    }

    D_WO_CHECK(sink.size() == 22u);

    return true;
}

/*
tests_write_only_clear_empties_the_sink
  the reset operation.
  Tests the following:
  - clear removes every element, so the count returns to zero
  - the sink reports empty afterwards
  - clearing an already-empty sink is harmless
  - the sink remains usable
*/
bool
tests_write_only_clear_empties_the_sink()
{
    dj::write_only_container<std::vector<int> > sink(std::size_t(5), 7);

    D_WO_CHECK(sink.size() == 5u);

    sink.clear();
    D_WO_CHECK(sink.size() == 0u);
    D_WO_CHECK(sink.empty());

    // clearing again is harmless
    sink.clear();
    D_WO_CHECK(sink.empty());

    // and the sink still works
    sink.push_back(1);
    D_WO_CHECK(sink.size() == 1u);

    // the held elements really are destroyed, which an instance-counting
    // element type can show without any read access
    {
        dj::write_only_container<std::vector<wo_probe_type> > objs;

        objs.emplace_back(1, 'a');
        objs.emplace_back(2, 'b');

        const int live_before = wo_probe_type::s_live;

        D_WO_CHECK(objs.size() == 2u);
        D_WO_CHECK(live_before >= 2);

        objs.clear();

        D_WO_CHECK(objs.empty());
        D_WO_CHECK(wo_probe_type::s_live == (live_before - 2));
    }

    // a node container and a character container
    dj::write_only_container<std::list<int> > l(std::size_t(3), 1);

    l.clear();
    D_WO_CHECK(l.empty());

    dj::write_only_container<std::string> s(std::size_t(4), 'a');

    s.clear();
    D_WO_CHECK(s.empty());

    return true;
}

/*
tests_write_only_append_and_reset_cycle
  the usage pattern the overlay exists for.
  Tests the following:
  - put-in and reset can be repeated indefinitely
  - each cycle starts from an empty sink and ends where it began
  - the counts through a cycle are exactly what was put in
*/
bool
tests_write_only_append_and_reset_cycle()
{
    dj::write_only_container<std::vector<int> > sink;
    std::size_t                                 cycle = 0;

    for (cycle = 0; cycle < 5u; ++cycle)
    {
        D_WO_CHECK(sink.empty());

        std::size_t i = 0;

        for (i = 0; i <= cycle; ++i)
        {
            sink.push_back(static_cast<int>(i));
        }

        if (sink.size() != (cycle + 1u))
        {
            std::printf("    [FAIL] cycle %u: size %u, expected %u\n",
                        static_cast<unsigned int>(cycle),
                        static_cast<unsigned int>(sink.size()),
                        static_cast<unsigned int>(cycle + 1u));

            return false;
        }

        sink.clear();
        D_WO_CHECK(sink.empty());
        D_WO_CHECK(sink.size() == 0u);
    }

    // mixing the two append forms within a cycle
    sink.push_back(1);
    sink.emplace_back(2);
    sink.push_back(3);
    D_WO_CHECK(sink.size() == 3u);

    sink.clear();
    D_WO_CHECK(sink.empty());

    return true;
}

/*
tests_write_only_append_stores_the_element
  that the element really is held.
  Tests the following:
  - an appended element is constructed and kept alive by the sink, shown by
    an element type that counts its own live instances
  - destroying the sink destroys the elements it held
  - none of this requires reading an element back, which the overlay
    forbids
*/
bool
tests_write_only_append_stores_the_element()
{
    const int live_at_entry = wo_probe_type::s_live;

    {
        dj::write_only_container<std::vector<wo_probe_type> > sink;

        D_WO_CHECK(wo_probe_type::s_live == live_at_entry);

        sink.emplace_back(1, 'a');
        D_WO_CHECK(sink.size() == 1u);
        D_WO_CHECK(wo_probe_type::s_live == (live_at_entry + 1));

        sink.emplace_back(2, 'b');
        D_WO_CHECK(sink.size() == 2u);
        D_WO_CHECK(wo_probe_type::s_live == (live_at_entry + 2));

        // push_back of a prepared value stores a copy
        wo_probe_type value(3, 'c');

        sink.push_back(value);
        D_WO_CHECK(sink.size() == 3u);
        D_WO_CHECK(wo_probe_type::s_live == (live_at_entry + 4));
    }

    // the sink and the local value have both gone out of scope
    D_WO_CHECK(wo_probe_type::s_live == live_at_entry);

    return true;
}

/*
tests_write_only_append_surface_tracks_container
  the SFINAE guard on the append members.
  Tests the following:
  - each append member appears only when the held container offers it
  - a fixed-size container offers none of them, so its sink is an inert
    holder with only the metadata surface
  - the growable containers keep all three
*/
bool
tests_write_only_append_surface_tracks_container()
{
    // the growable containers keep all three
    typedef dj::write_only_container<std::vector<int> > vec_sink;
    typedef dj::write_only_container<std::list<int> >   list_sink;
    typedef dj::write_only_container<std::string>       str_sink;

    D_WO_CHECK(wo_has_push_back<vec_sink>::value);
    D_WO_CHECK(wo_has_emplace_back<vec_sink>::value);
    D_WO_CHECK(wo_has_clear<vec_sink>::value);

    D_WO_CHECK(wo_has_push_back<list_sink>::value);
    D_WO_CHECK(wo_has_emplace_back<list_sink>::value);
    D_WO_CHECK(wo_has_clear<list_sink>::value);

    D_WO_CHECK(wo_has_push_back<str_sink>::value);
    D_WO_CHECK(wo_has_clear<str_sink>::value);

    // a fixed-size container offers none of them
    typedef dj::write_only_container<std::array<int, 3> > arr_sink;

    D_WO_CHECK(!wo_has_push_back<arr_sink>::value);
    D_WO_CHECK(!wo_has_emplace_back<arr_sink>::value);
    D_WO_CHECK(!wo_has_clear<arr_sink>::value);

    // because the container does not
    D_WO_CHECK(!wo_has_push_back<std::array<int, 3> >::value);
    D_WO_CHECK(!wo_has_clear<std::array<int, 3> >::value);

    // so an array sink is inert: only the metadata surface remains, and it
    // still answers correctly
    const arr_sink a;

    D_WO_CHECK(wo_has_size<arr_sink>::value);
    D_WO_CHECK(wo_has_empty<arr_sink>::value);
    D_WO_CHECK(a.size() == 3u);
    D_WO_CHECK(!a.empty());

    // and it exposes no reader either, so it is a holder and nothing more
    D_WO_CHECK(!wo_any_reader(wo_probe<arr_sink>()));

    return true;
}

NS_END  // testing
NS_END  // djinterp
