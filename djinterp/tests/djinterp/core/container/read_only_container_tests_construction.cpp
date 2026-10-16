/*******************************************************************************
* djinterp [core]                     read_only_container_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_read_only_default_construction
  the no-argument form.
  Tests the following:
  - a default-constructed handle holds a default-constructed container
  - it reports empty, with size zero
  - it is available for containers that are themselves default-constructible
*/
bool
tests_read_only_default_construction()
{
    const dj::read_only_container<std::vector<int> > v;

    D_RO_CHECK(v.size() == 0u);
    D_RO_CHECK(v.empty());

    // a fixed-size container default-constructs to its own size
    const dj::read_only_container<std::array<int, 3> > a;

    D_RO_CHECK(a.size() == 3u);
    D_RO_CHECK(!a.empty());

    // a node container behaves like the vector
    const dj::read_only_container<std::list<int> > l;

    D_RO_CHECK(l.size() == 0u);
    D_RO_CHECK(l.empty());

    // and a character container
    const dj::read_only_container<std::string> s;

    D_RO_CHECK(s.size() == 0u);
    D_RO_CHECK(s.empty());

    return true;
}

/*
tests_read_only_wraps_a_container_value
  the single-argument form.
  Tests the following:
  - a container handed over by value is held intact, element for element
  - an lvalue container is copied rather than consumed
  - an rvalue container may be moved from
*/
bool
tests_read_only_wraps_a_container_value()
{
    std::vector<int> src;

    src.push_back(10);
    src.push_back(20);
    src.push_back(30);

    // from an lvalue: the source survives
    const dj::read_only_container<std::vector<int> > copied(src);

    D_RO_CHECK(copied.size() == 3u);
    D_RO_CHECK(copied[0] == 10);
    D_RO_CHECK(copied[2] == 30);
    D_RO_CHECK(src.size() == 3u);

    // from an rvalue
    std::vector<int>                                spare(src);
    const dj::read_only_container<std::vector<int> > moved(
        static_cast<std::vector<int>&&>(spare));

    D_RO_CHECK(moved.size() == 3u);
    D_RO_CHECK(moved[1] == 20);

    // from a const lvalue
    const std::vector<int>&                         cref = src;
    const dj::read_only_container<std::vector<int> > from_const(cref);

    D_RO_CHECK(from_const.size() == 3u);
    D_RO_CHECK(from_const[0] == 10);

    return true;
}

/*
tests_read_only_emplaces_from_arguments
  the multi-argument form.
  Tests the following:
  - several arguments construct the held container in place
  - each argument reaches the container's own constructor unchanged
  - an iterator pair is forwarded as readily as a count and a value
*/
bool
tests_read_only_emplaces_from_arguments()
{
    // count and value
    const dj::read_only_container<std::vector<int> > filled(
        std::size_t(4), 7);

    D_RO_CHECK(filled.size() == 4u);
    D_RO_CHECK(filled[0] == 7);
    D_RO_CHECK(filled[3] == 7);

    // the two arguments are not transposed
    const dj::read_only_container<std::vector<int> > other(
        std::size_t(2), 9);

    D_RO_CHECK(other.size() == 2u);
    D_RO_CHECK(other[0] == 9);

    // an iterator pair
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);
    src.push_back(3);

    const dj::read_only_container<std::vector<int> > ranged(
        src.begin(), src.end());

    D_RO_CHECK(ranged.size() == 3u);
    D_RO_CHECK(ranged[0] == 1);
    D_RO_CHECK(ranged[2] == 3);

    // a character container built from a count and a character
    const dj::read_only_container<std::string> text(std::size_t(3), 'x');

    D_RO_CHECK(text.size() == 3u);
    D_RO_CHECK(text[0] == 'x');
    D_RO_CHECK(text[2] == 'x');

    return true;
}

/*
tests_read_only_copy_and_move_construction
  handle-to-handle construction.
  Tests the following:
  - a handle can be copy-constructed, and the copy observes the same values
  - a handle can be move-constructed
  - copy assignment is likewise available and preserves the elements
*/
bool
tests_read_only_copy_and_move_construction()
{
    std::vector<int> src;

    src.push_back(4);
    src.push_back(5);

    dj::read_only_container<std::vector<int> > a(src);

    // copy
    const dj::read_only_container<std::vector<int> > b(a);

    D_RO_CHECK(b.size() == 2u);
    D_RO_CHECK(b[0] == 4);
    D_RO_CHECK(b[1] == 5);

    // from a const lvalue handle
    const dj::read_only_container<std::vector<int> >& aref = a;
    const dj::read_only_container<std::vector<int> >  c(aref);

    D_RO_CHECK(c.size() == 2u);
    D_RO_CHECK(c[0] == 4);

    // move
    const dj::read_only_container<std::vector<int> > d(
        static_cast<dj::read_only_container<std::vector<int> >&&>(a));

    D_RO_CHECK(d.size() == 2u);
    D_RO_CHECK(d[1] == 5);

    // assignment
    dj::read_only_container<std::vector<int> > e;

    e = b;
    D_RO_CHECK(e.size() == 2u);
    D_RO_CHECK(e[0] == 4);

    // the traits agree that both operations are available
    typedef dj::read_only_container<std::vector<int> > handle;

    D_RO_CHECK(std::is_copy_constructible<handle>::value);
    D_RO_CHECK(std::is_move_constructible<handle>::value);

    return true;
}

/*
tests_read_only_variadic_ctor_excludes_self
  the enable_if guard on the emplacing constructor.
  Tests the following:
  - copying a handle selects the copy constructor, not the variadic one -
    were the guard missing, the variadic form would try to build the held
    container FROM a handle and fail to compile
  - the guard strips reference and cv qualification, so it fires for a
    non-const lvalue, a const lvalue and an rvalue alike
  - a single argument of a DIFFERENT type still reaches the variadic form
*/
bool
tests_read_only_variadic_ctor_excludes_self()
{
    typedef dj::read_only_container<std::vector<int> > handle;

    std::vector<int> src;

    src.push_back(8);

    handle       original(src);
    const handle const_original(src);

    // all three reference categories select the copy or move constructor;
    // that these compile at all is the assertion
    const handle from_lvalue(original);
    const handle from_const_lvalue(const_original);
    const handle from_rvalue(static_cast<handle&&>(original));

    D_RO_CHECK(from_lvalue.size() == 1u);
    D_RO_CHECK(from_const_lvalue.size() == 1u);
    D_RO_CHECK(from_rvalue.size() == 1u);
    D_RO_CHECK(from_lvalue[0] == 8);
    D_RO_CHECK(from_const_lvalue[0] == 8);

    // a single argument of a different type still emplaces
    const handle from_container(src);

    D_RO_CHECK(from_container.size() == 1u);
    D_RO_CHECK(from_container[0] == 8);

    // the guard keys on the stripped type, which is what makes the three
    // categories above behave alike
    D_RO_CHECK((std::is_same<dj::clean_t<handle&>, handle>::value));
    D_RO_CHECK((std::is_same<dj::clean_t<const handle&>, handle>::value));
    D_RO_CHECK((std::is_same<dj::clean_t<handle&&>, handle>::value));

    return true;
}

/*
tests_read_only_factory_deduces_container
  make_read_only.
  Tests the following:
  - the returned handle is parameterised on the cleaned container type
  - deduction works from an lvalue, a const lvalue and an rvalue
  - the wrapped elements survive the trip
*/
bool
tests_read_only_factory_deduces_container()
{
    std::vector<int> src;

    src.push_back(2);
    src.push_back(4);

    typedef dj::read_only_container<std::vector<int> > expected;

    // from an lvalue
    const expected a = dj::make_read_only(src);

    D_RO_CHECK((std::is_same<decltype(dj::make_read_only(src)),
                             expected>::value));
    D_RO_CHECK(a.size() == 2u);
    D_RO_CHECK(a[1] == 4);

    // from a const lvalue
    const std::vector<int>& cref = src;

    D_RO_CHECK((std::is_same<decltype(dj::make_read_only(cref)),
                             expected>::value));
    D_RO_CHECK(dj::make_read_only(cref).size() == 2u);

    // from an rvalue
    D_RO_CHECK((std::is_same<
                    decltype(dj::make_read_only(std::vector<int>())),
                    expected>::value));

    // the source lvalue is copied, not consumed
    D_RO_CHECK(src.size() == 2u);

    // and the factory works for other containers
    std::list<int> nodes;

    nodes.push_back(6);

    D_RO_CHECK((std::is_same<
                    decltype(dj::make_read_only(nodes)),
                    dj::read_only_container<std::list<int> > >::value));
    D_RO_CHECK(dj::make_read_only(nodes).size() == 1u);
    D_RO_CHECK(dj::make_read_only(std::string("abc")).size() == 3u);

    return true;
}

/*
tests_read_only_holds_an_independent_copy
  ownership by value.
  Tests the following:
  - the handle holds its container as a member rather than referring to the
    caller's object
  - a later change to the source container is NOT observed through the
    handle, which is what makes the seal meaningful
  - two handles built from one source are independent of each other
*/
bool
tests_read_only_holds_an_independent_copy()
{
    std::vector<int> src;

    src.push_back(1);
    src.push_back(2);

    const dj::read_only_container<std::vector<int> > handle(src);

    D_RO_CHECK(handle.size() == 2u);

    // change the source afterwards
    src.push_back(3);
    src[0] = 99;

    // the handle is unaffected
    D_RO_CHECK(handle.size() == 2u);
    D_RO_CHECK(handle[0] == 1);
    D_RO_CHECK(handle[1] == 2);

    // its storage is not the source's storage
    D_RO_CHECK(handle.data() != src.data());

    // two handles from one source are independent
    std::vector<int> shared;

    shared.push_back(7);

    const dj::read_only_container<std::vector<int> > h1(shared);
    const dj::read_only_container<std::vector<int> > h2(shared);

    D_RO_CHECK(h1.data() != h2.data());
    D_RO_CHECK(h1[0] == h2[0]);

    return true;
}

NS_END  // testing
NS_END  // djinterp
