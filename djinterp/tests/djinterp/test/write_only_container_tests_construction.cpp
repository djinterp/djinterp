/*******************************************************************************
* djinterp [test]                    write_only_container_tests_construction.cpp
*
*
* path:      /tests/djinterp/test/write_only_container_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "../core/container/write_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// the instance counters the probe element type keeps
int wo_probe_type::s_live        = 0;
int wo_probe_type::s_constructed = 0;


/*
tests_write_only_default_construction
  the no-argument form.
  Tests the following:
  - a default-constructed sink holds a default-constructed container
  - it reports empty, with size zero, for the growable containers
  - a fixed-size container default-constructs to its own size
  - the sink is immediately usable for appending
*/
bool
tests_write_only_default_construction()
{
    const dj::write_only_container<std::vector<int> > v;

    D_WO_CHECK(v.size() == 0u);
    D_WO_CHECK(v.empty());

    const dj::write_only_container<std::list<int> > l;

    D_WO_CHECK(l.size() == 0u);
    D_WO_CHECK(l.empty());

    const dj::write_only_container<std::string> s;

    D_WO_CHECK(s.size() == 0u);
    D_WO_CHECK(s.empty());

    // a fixed-size container reports its own size
    const dj::write_only_container<std::array<int, 3> > a;

    D_WO_CHECK(a.size() == 3u);
    D_WO_CHECK(!a.empty());

    // and a default-constructed sink accepts an append at once
    dj::write_only_container<std::vector<int> > w;

    w.push_back(1);
    D_WO_CHECK(w.size() == 1u);
    D_WO_CHECK(!w.empty());

    return true;
}

/*
tests_write_only_wraps_a_container_value
  the single-argument form.
  Tests the following:
  - a container handed over at construction is held, and its element count
    is reported
  - an lvalue container is copied rather than consumed
  - an rvalue container may be moved from
*/
bool
tests_write_only_wraps_a_container_value()
{
    std::vector<int> src;

    src.push_back(10);
    src.push_back(20);
    src.push_back(30);

    const dj::write_only_container<std::vector<int> > copied(src);

    D_WO_CHECK(copied.size() == 3u);
    D_WO_CHECK(!copied.empty());
    D_WO_CHECK(src.size() == 3u);

    // from an rvalue
    std::vector<int>                                 spare(src);
    const dj::write_only_container<std::vector<int> > moved(
        static_cast<std::vector<int>&&>(spare));

    D_WO_CHECK(moved.size() == 3u);

    // from a const lvalue
    const std::vector<int>&                          cref = src;
    const dj::write_only_container<std::vector<int> > from_const(cref);

    D_WO_CHECK(from_const.size() == 3u);

    // a node container and a character container
    std::list<int> nodes(std::size_t(2), 5);

    D_WO_CHECK(dj::write_only_container<std::list<int> >(nodes).size() == 2u);
    D_WO_CHECK(dj::write_only_container<std::string>(
                   std::string("abcd")).size() == 4u);

    return true;
}

/*
tests_write_only_emplaces_from_arguments
  the multi-argument form.
  Tests the following:
  - several arguments construct the held container in place
  - each argument reaches the container's own constructor unchanged
  - an iterator pair is forwarded as readily as a count and a value
*/
bool
tests_write_only_emplaces_from_arguments()
{
    const dj::write_only_container<std::vector<int> > filled(
        std::size_t(4), 7);

    D_WO_CHECK(filled.size() == 4u);
    D_WO_CHECK(!filled.empty());

    // a different count gives a different size, so the first argument is
    // the one being used
    D_WO_CHECK(dj::write_only_container<std::vector<int> >(
                   std::size_t(2), 9).size() == 2u);
    D_WO_CHECK(dj::write_only_container<std::vector<int> >(
                   std::size_t(0), 9).size() == 0u);

    // an iterator pair
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);
    src.push_back(3);

    const dj::write_only_container<std::vector<int> > ranged(
        src.begin(), src.end());

    D_WO_CHECK(ranged.size() == 3u);

    // a character container
    const dj::write_only_container<std::string> text(std::size_t(3), 'x');

    D_WO_CHECK(text.size() == 3u);

    // and the emplaced sink still accepts appends
    dj::write_only_container<std::vector<int> > w(std::size_t(2), 5);

    w.push_back(6);
    D_WO_CHECK(w.size() == 3u);

    return true;
}

/*
tests_write_only_copy_and_move_construction
  handle-to-handle construction.
  Tests the following:
  - a sink can be copy-constructed and move-constructed
  - the copy holds its own container, so appending to one does not touch
    the other
  - copy assignment is likewise available
*/
bool
tests_write_only_copy_and_move_construction()
{
    std::vector<int> src;

    src.push_back(4);
    src.push_back(5);

    dj::write_only_container<std::vector<int> > a(src);
    dj::write_only_container<std::vector<int> > b(a);

    D_WO_CHECK(b.size() == 2u);

    // the copy is independent
    b.push_back(6);
    D_WO_CHECK(b.size() == 3u);
    D_WO_CHECK(a.size() == 2u);

    // move
    dj::write_only_container<std::vector<int> > c(
        static_cast<dj::write_only_container<std::vector<int> >&&>(b));

    D_WO_CHECK(c.size() == 3u);

    // assignment
    dj::write_only_container<std::vector<int> > d;

    d = c;
    D_WO_CHECK(d.size() == 3u);
    d.push_back(7);
    D_WO_CHECK(d.size() == 4u);
    D_WO_CHECK(c.size() == 3u);

    typedef dj::write_only_container<std::vector<int> > handle;

    D_WO_CHECK(std::is_copy_constructible<handle>::value);
    D_WO_CHECK(std::is_move_constructible<handle>::value);

    return true;
}

/*
tests_write_only_variadic_ctor_excludes_self
  the enable_if guard on the emplacing constructor.
  Tests the following:
  - copying a sink selects the copy constructor, not the variadic one -
    were the guard missing, the variadic form would try to build the held
    container FROM a sink and fail to compile
  - the guard strips reference and cv qualification, so it fires for a
    non-const lvalue, a const lvalue and an rvalue alike
*/
bool
tests_write_only_variadic_ctor_excludes_self()
{
    typedef dj::write_only_container<std::vector<int> > handle;

    std::vector<int> src;

    src.push_back(8);

    handle       original(src);
    const handle const_original(src);

    const handle from_lvalue(original);
    const handle from_const_lvalue(const_original);
    const handle from_rvalue(static_cast<handle&&>(original));

    D_WO_CHECK(from_lvalue.size() == 1u);
    D_WO_CHECK(from_const_lvalue.size() == 1u);
    D_WO_CHECK(from_rvalue.size() == 1u);

    // a single argument of a different type still emplaces
    const handle from_container(src);

    D_WO_CHECK(from_container.size() == 1u);

    // the guard keys on the stripped type
    D_WO_CHECK((std::is_same<dj::clean_t<handle&>, handle>::value));
    D_WO_CHECK((std::is_same<dj::clean_t<const handle&>, handle>::value));
    D_WO_CHECK((std::is_same<dj::clean_t<handle&&>, handle>::value));

    return true;
}

/*
tests_write_only_factory_deduces_container
  make_write_only.
  Tests the following:
  - the returned sink is parameterised on the cleaned container type
  - deduction works from an lvalue, a const lvalue and an rvalue
  - the resulting sink accepts appends
*/
bool
tests_write_only_factory_deduces_container()
{
    std::vector<int> src;

    src.push_back(2);
    src.push_back(4);

    typedef dj::write_only_container<std::vector<int> > expected;

    D_WO_CHECK((std::is_same<decltype(dj::make_write_only(src)),
                             expected>::value));

    const std::vector<int>& cref = src;

    D_WO_CHECK((std::is_same<decltype(dj::make_write_only(cref)),
                             expected>::value));
    D_WO_CHECK((std::is_same<
                    decltype(dj::make_write_only(std::vector<int>())),
                    expected>::value));

    expected h = dj::make_write_only(src);

    D_WO_CHECK(h.size() == 2u);
    h.push_back(6);
    D_WO_CHECK(h.size() == 3u);

    // the source lvalue is copied, not consumed
    D_WO_CHECK(src.size() == 2u);

    // other containers deduce too
    std::list<int> nodes;

    nodes.push_back(6);

    D_WO_CHECK((std::is_same<
                    decltype(dj::make_write_only(nodes)),
                    dj::write_only_container<std::list<int> > >::value));
    D_WO_CHECK(dj::make_write_only(std::string("abc")).size() == 3u);

    return true;
}

/*
tests_write_only_holds_an_independent_copy
  ownership by value.
  Tests the following:
  - the sink holds its container as a member rather than referring to the
    caller's object
  - appends through the sink do NOT reach the source container
  - later changes to the source are not observed through the sink
*/
bool
tests_write_only_holds_an_independent_copy()
{
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);

    dj::write_only_container<std::vector<int> > sink(src);

    // append through the sink
    sink.push_back(3);
    sink.push_back(4);

    // the source is untouched
    D_WO_CHECK(src.size() == 2u);
    D_WO_CHECK(src[0] == 1);
    D_WO_CHECK(src[1] == 2);

    // and the sink carries the appends
    D_WO_CHECK(sink.size() == 4u);

    // a later change to the source is not observed
    src.push_back(5);
    D_WO_CHECK(sink.size() == 4u);

    // clearing the sink leaves the source alone
    sink.clear();
    D_WO_CHECK(sink.empty());
    D_WO_CHECK(src.size() == 3u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
