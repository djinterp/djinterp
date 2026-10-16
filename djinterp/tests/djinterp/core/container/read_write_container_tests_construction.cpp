/*******************************************************************************
* djinterp [core]                    read_write_container_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_write_default_construction
  the no-argument form.
  Tests the following:
  - a default-constructed handle holds a default-constructed container
  - it reports empty, with size zero, for the growable containers
  - a fixed-size container default-constructs to its own size
*/
bool
tests_read_write_default_construction()
{
    const dj::read_write_container<std::vector<int> > v;

    D_RW_CHECK(v.size() == 0u);
    D_RW_CHECK(v.empty());

    const dj::read_write_container<std::array<int, 3> > a;

    D_RW_CHECK(a.size() == 3u);
    D_RW_CHECK(!a.empty());

    const dj::read_write_container<std::list<int> > l;

    D_RW_CHECK(l.size() == 0u);
    D_RW_CHECK(l.empty());

    const dj::read_write_container<std::string> s;

    D_RW_CHECK(s.size() == 0u);
    D_RW_CHECK(s.empty());

    // and a default-constructed handle is immediately usable for writing
    dj::read_write_container<std::vector<int> > w;

    w.push_back(1);
    D_RW_CHECK(w.size() == 1u);
    D_RW_CHECK(w[0] == 1);

    return true;
}

/*
tests_read_write_wraps_a_container_value
  the single-argument form.
  Tests the following:
  - a container handed over by value is held intact, element for element
  - an lvalue container is copied rather than consumed
  - an rvalue container may be moved from
*/
bool
tests_read_write_wraps_a_container_value()
{
    std::vector<int> src;

    src.push_back(10);
    src.push_back(20);
    src.push_back(30);

    const dj::read_write_container<std::vector<int> > copied(src);

    D_RW_CHECK(copied.size() == 3u);
    D_RW_CHECK(copied[0] == 10);
    D_RW_CHECK(copied[2] == 30);
    D_RW_CHECK(src.size() == 3u);

    // from an rvalue
    std::vector<int>                                 spare(src);
    const dj::read_write_container<std::vector<int> > moved(
        static_cast<std::vector<int>&&>(spare));

    D_RW_CHECK(moved.size() == 3u);
    D_RW_CHECK(moved[1] == 20);

    // from a const lvalue
    const std::vector<int>&                          cref = src;
    const dj::read_write_container<std::vector<int> > from_const(cref);

    D_RW_CHECK(from_const.size() == 3u);
    D_RW_CHECK(from_const[0] == 10);

    return true;
}

/*
tests_read_write_emplaces_from_arguments
  the multi-argument form.
  Tests the following:
  - several arguments construct the held container in place
  - each argument reaches the container's own constructor unchanged
  - an iterator pair is forwarded as readily as a count and a value
*/
bool
tests_read_write_emplaces_from_arguments()
{
    const dj::read_write_container<std::vector<int> > filled(
        std::size_t(4), 7);

    D_RW_CHECK(filled.size() == 4u);
    D_RW_CHECK(filled[0] == 7);
    D_RW_CHECK(filled[3] == 7);

    // the arguments are not transposed
    const dj::read_write_container<std::vector<int> > other(
        std::size_t(2), 9);

    D_RW_CHECK(other.size() == 2u);
    D_RW_CHECK(other[0] == 9);

    // an iterator pair
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);
    src.push_back(3);

    const dj::read_write_container<std::vector<int> > ranged(
        src.begin(), src.end());

    D_RW_CHECK(ranged.size() == 3u);
    D_RW_CHECK(ranged[0] == 1);
    D_RW_CHECK(ranged[2] == 3);

    // a character container
    const dj::read_write_container<std::string> text(std::size_t(3), 'x');

    D_RW_CHECK(text.size() == 3u);
    D_RW_CHECK(text[0] == 'x');

    // and the emplaced handle is writable
    dj::read_write_container<std::vector<int> > w(std::size_t(2), 5);

    w.push_back(6);
    D_RW_CHECK(w.size() == 3u);
    D_RW_CHECK(w[2] == 6);

    return true;
}

/*
tests_read_write_copy_and_move_construction
  handle-to-handle construction.
  Tests the following:
  - a handle can be copy-constructed and move-constructed
  - the copy holds its own container, so mutating one does not touch the
    other
  - copy assignment is likewise available
*/
bool
tests_read_write_copy_and_move_construction()
{
    std::vector<int> src;

    src.push_back(4);
    src.push_back(5);

    dj::read_write_container<std::vector<int> > a(src);
    dj::read_write_container<std::vector<int> > b(a);

    D_RW_CHECK(b.size() == 2u);
    D_RW_CHECK(b[0] == 4);

    // the copy is independent
    b.push_back(6);
    D_RW_CHECK(b.size() == 3u);
    D_RW_CHECK(a.size() == 2u);

    a[0] = 99;
    D_RW_CHECK(b[0] == 4);

    // move
    dj::read_write_container<std::vector<int> > c(
        static_cast<dj::read_write_container<std::vector<int> >&&>(b));

    D_RW_CHECK(c.size() == 3u);
    D_RW_CHECK(c[2] == 6);

    // assignment
    dj::read_write_container<std::vector<int> > d;

    d = c;
    D_RW_CHECK(d.size() == 3u);
    d.push_back(7);
    D_RW_CHECK(d.size() == 4u);
    D_RW_CHECK(c.size() == 3u);

    typedef dj::read_write_container<std::vector<int> > handle;

    D_RW_CHECK(std::is_copy_constructible<handle>::value);
    D_RW_CHECK(std::is_move_constructible<handle>::value);

    return true;
}

/*
tests_read_write_variadic_ctor_excludes_self
  the enable_if guard on the emplacing constructor.
  Tests the following:
  - copying a handle selects the copy constructor, not the variadic one -
    were the guard missing, the variadic form would try to build the held
    container FROM a handle and fail to compile
  - the guard strips reference and cv qualification, so it fires for a
    non-const lvalue, a const lvalue and an rvalue alike
*/
bool
tests_read_write_variadic_ctor_excludes_self()
{
    typedef dj::read_write_container<std::vector<int> > handle;

    std::vector<int> src;

    src.push_back(8);

    handle       original(src);
    const handle const_original(src);

    const handle from_lvalue(original);
    const handle from_const_lvalue(const_original);
    const handle from_rvalue(static_cast<handle&&>(original));

    D_RW_CHECK(from_lvalue.size() == 1u);
    D_RW_CHECK(from_const_lvalue.size() == 1u);
    D_RW_CHECK(from_rvalue.size() == 1u);
    D_RW_CHECK(from_lvalue[0] == 8);

    // a single argument of a different type still emplaces
    const handle from_container(src);

    D_RW_CHECK(from_container.size() == 1u);

    // the guard keys on the stripped type
    D_RW_CHECK((std::is_same<dj::clean_t<handle&>, handle>::value));
    D_RW_CHECK((std::is_same<dj::clean_t<const handle&>, handle>::value));
    D_RW_CHECK((std::is_same<dj::clean_t<handle&&>, handle>::value));

    return true;
}

/*
tests_read_write_factory_deduces_container
  make_read_write.
  Tests the following:
  - the returned handle is parameterised on the cleaned container type
  - deduction works from an lvalue, a const lvalue and an rvalue
  - the resulting handle is writable
*/
bool
tests_read_write_factory_deduces_container()
{
    std::vector<int> src;

    src.push_back(2);
    src.push_back(4);

    typedef dj::read_write_container<std::vector<int> > expected;

    D_RW_CHECK((std::is_same<decltype(dj::make_read_write(src)),
                             expected>::value));

    const std::vector<int>& cref = src;

    D_RW_CHECK((std::is_same<decltype(dj::make_read_write(cref)),
                             expected>::value));
    D_RW_CHECK((std::is_same<
                    decltype(dj::make_read_write(std::vector<int>())),
                    expected>::value));

    // the wrapped elements survive and the handle is writable
    expected h = dj::make_read_write(src);

    D_RW_CHECK(h.size() == 2u);
    D_RW_CHECK(h[1] == 4);
    h.push_back(6);
    D_RW_CHECK(h.size() == 3u);

    // the source lvalue is copied, not consumed
    D_RW_CHECK(src.size() == 2u);

    // other containers deduce too
    std::list<int> nodes;

    nodes.push_back(6);

    D_RW_CHECK((std::is_same<
                    decltype(dj::make_read_write(nodes)),
                    dj::read_write_container<std::list<int> > >::value));
    D_RW_CHECK(dj::make_read_write(std::string("abc")).size() == 3u);

    return true;
}

/*
tests_read_write_holds_an_independent_copy
  ownership by value.
  Tests the following:
  - the handle holds its container as a member rather than referring to the
    caller's object
  - writes through the handle do NOT reach the source container
  - later changes to the source are not observed through the handle
*/
bool
tests_read_write_holds_an_independent_copy()
{
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);

    dj::read_write_container<std::vector<int> > handle(src);

    // write through the handle
    handle[0] = 99;
    handle.push_back(3);

    // the source is untouched
    D_RW_CHECK(src.size() == 2u);
    D_RW_CHECK(src[0] == 1);

    // and the handle carries the change
    D_RW_CHECK(handle.size() == 3u);
    D_RW_CHECK(handle[0] == 99);

    // a later change to the source is not observed
    src.push_back(4);
    src[1] = 77;

    D_RW_CHECK(handle.size() == 3u);
    D_RW_CHECK(handle[1] == 2);

    // the two do not share storage
    D_RW_CHECK(handle.data() != src.data());

    return true;
}

NS_END  // testing
NS_END  // djinterp
