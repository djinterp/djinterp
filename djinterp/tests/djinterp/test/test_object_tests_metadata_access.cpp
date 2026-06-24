// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- metadata access                              ///
///////////////////////////////////////////////////////////////////////////////

// metadata() overloads return references to the container type
static_assert(std::is_same<decltype(std::declval<basic_test&>().metadata()),
                           test_metadata&>::value, "");
static_assert(std::is_same<
                  decltype(std::declval<const basic_test&>().metadata()),
                  const test_metadata&>::value, "");

// set_metadata(const&) is never noexcept; set_metadata(&&) is noexcept iff the
// container's move assignment is
static_assert(!noexcept(std::declval<basic_test&>().set_metadata(
                  std::declval<const test_metadata&>())), "");
static_assert(noexcept(std::declval<basic_test&>().set_metadata(
                  std::declval<test_metadata&&>())), "");
static_assert(!noexcept(std::declval<throw_object&>().set_metadata(
                  std::declval<throwmeta&&>())), "");

// constexpr metadata() const (literal container)
namespace
{
    constexpr lit_object k_meta(5);
}
static_assert(k_meta.metadata().tag == 0, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- metadata access                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_object_metadata_accessor
  Verifies the mutable and const metadata() accessors.
  Tests the following:
  - the non-const accessor yields a mutable reference whose edits persist
  - the const accessor observes those same edits
  - both overloads name the very same stored sub-object (address identity)
  - a vector-backed container is reachable and mutable the same way
*/
bool
tests_object_metadata_accessor()
{
    bool ok = true;

    basic_test t;

    // mutate through the non-const reference
    t.metadata().set("name", "alpha");
    ok = D_TO_CHECK(t.metadata().get("name") == "alpha") && ok;

    // observe through the const reference
    const basic_test& ct = t;
    ok = D_TO_CHECK(ct.metadata().get("name") == "alpha") && ok;
    ok = D_TO_CHECK(ct.metadata().size() == 1u)           && ok;

    // both overloads refer to the same sub-object
    ok = D_TO_CHECK(&t.metadata() == &ct.metadata()) && ok;

    // vector-backed container
    {
        tagged_test g(1, true);
        g.metadata().push_back(7);
        g.metadata().push_back(8);
        ok = D_TO_CHECK(g.metadata().size() == 2u) && ok;
        ok = D_TO_CHECK(g.metadata()[0] == 7)      && ok;
        ok = D_TO_CHECK(g.metadata()[1] == 8)      && ok;
    }

    return ok;
}


/*
tests_object_set_metadata_copy
  Verifies set_metadata(const&).
  Tests the following:
  - the supplied container is copied in and becomes observable
  - the source container is left intact
  - an assignment replaces any previously-held metadata wholesale
*/
bool
tests_object_set_metadata_copy()
{
    bool ok = true;

    basic_test t;

    test_metadata m;
    m.set("name", "alpha");
    m.set("message", "hi");

    t.set_metadata(m);
    ok = D_TO_CHECK(t.metadata().size() == 2u)            && ok;
    ok = D_TO_CHECK(t.metadata().get("name") == "alpha")  && ok;
    ok = D_TO_CHECK(t.metadata().get("message") == "hi")  && ok;
    // source intact (copy)
    ok = D_TO_CHECK(m.size() == 2u)            && ok;
    ok = D_TO_CHECK(m.get("name") == "alpha")  && ok;

    // a second assignment replaces the container wholesale
    test_metadata m2;
    m2.set("only", "one");
    t.set_metadata(m2);
    ok = D_TO_CHECK(t.metadata().size() == 1u)           && ok;
    ok = D_TO_CHECK(t.metadata().get("only") == "one")   && ok;
    ok = D_TO_CHECK(t.metadata().contains("name") == false) && ok;

    return ok;
}


/*
tests_object_set_metadata_move
  Verifies set_metadata(&&).
  Tests the following:
  - the supplied container's contents are transferred into the object
  - status/type/result are unaffected by a metadata assignment
*/
bool
tests_object_set_metadata_move()
{
    bool ok = true;

    basic_test t(4, true);

    test_metadata m;
    m.set("k", "v");
    m.set("k2", "v2");

    t.set_metadata(std::move(m));
    ok = D_TO_CHECK(t.metadata().size() == 2u)         && ok;
    ok = D_TO_CHECK(t.metadata().get("k") == "v")      && ok;
    ok = D_TO_CHECK(t.metadata().get("k2") == "v2")    && ok;
    // unrelated fields untouched
    ok = D_TO_CHECK(t.type_id() == 4)                  && ok;
    ok = D_TO_CHECK(t.passed())                        && ok;

    return ok;
}


/*
tests_object_metadata_noexcept
  Verifies the set_metadata exception specifications.
  Tests the following:
  - the copy overload is never noexcept (it may allocate)
  - the move overload is noexcept iff the container's move assignment is
*/
bool
tests_object_metadata_noexcept()
{
    bool ok = true;

    ok = D_TO_CHECK(!noexcept(std::declval<basic_test&>().set_metadata(
                        std::declval<const test_metadata&>()))) && ok;
    ok = D_TO_CHECK(noexcept(std::declval<basic_test&>().set_metadata(
                        std::declval<test_metadata&&>()))) && ok;
    ok = D_TO_CHECK(!noexcept(std::declval<throw_object&>().set_metadata(
                        std::declval<throwmeta&&>()))) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
