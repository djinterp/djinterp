// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- III. set construction / aliases              ///
///////////////////////////////////////////////////////////////////////////////

// forwarded type aliases (id-keyed set of ids)
static_assert(std::is_same<test_kind_set<id_set>::container_type,
                           id_set>::value, "");
static_assert(std::is_same<test_kind_set<id_set>::key_type,
                           test_type_id>::value, "");
static_assert(std::is_same<test_kind_set<id_set>::value_type,
                           test_type_id>::value, "");
static_assert(std::is_same<test_kind_set<id_set>::size_type,
                           std::size_t>::value, "");
static_assert(std::is_same<test_kind_set<id_set>::iterator,
                           id_set::iterator>::value, "");
static_assert(std::is_same<test_kind_set<id_set>::const_iterator,
                           id_set::const_iterator>::value, "");

// a set whose value_type is the record itself, keyed by id
static_assert(std::is_same<test_kind_set<kind_set>::value_type,
                           test_kind>::value, "");
static_assert(std::is_same<test_kind_set<kind_set>::key_type,
                           test_type_id>::value, "");

// construction exception specs: default & move(container) are noexcept; the
// copy(container) form is not (copying the container may allocate)
static_assert(std::is_nothrow_default_constructible<
                  test_kind_set<id_set> >::value, "");
static_assert(std::is_nothrow_constructible<
                  test_kind_set<id_set>, id_set&&>::value, "");
static_assert(!std::is_nothrow_constructible<
                  test_kind_set<id_set>, const id_set&>::value, "");

// the container constructors are explicit (constructible, not convertible)
static_assert(std::is_constructible<
                  test_kind_set<id_set>, const id_set&>::value, "");
static_assert(!std::is_convertible<
                  const id_set&, test_kind_set<id_set> >::value, "");
static_assert(!std::is_convertible<
                  id_set&&, test_kind_set<id_set> >::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- III. set construction / aliases                        ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_set_aliases
  Verifies the wrapper's forwarded type aliases.
  Tests the following:
  - container_type / key_type / value_type / size_type / iterator /
    const_iterator all name the underlying container's corresponding types
  - a record-valued set reports value_type == test_kind while still keyed by id
*/
bool
tests_set_aliases()
{
    bool ok = true;

    ok = D_TK_CHECK(std::is_same<test_kind_set<id_set>::container_type,
                                 id_set>::value) && ok;
    ok = D_TK_CHECK(std::is_same<test_kind_set<id_set>::key_type,
                                 test_type_id>::value) && ok;
    ok = D_TK_CHECK(std::is_same<test_kind_set<id_set>::value_type,
                                 test_type_id>::value) && ok;
    ok = D_TK_CHECK(std::is_same<test_kind_set<id_set>::size_type,
                                 std::size_t>::value) && ok;
    ok = D_TK_CHECK(std::is_same<test_kind_set<kind_set>::value_type,
                                 test_kind>::value) && ok;

    return ok;
}


/*
tests_set_default_ctor
  Verifies the default constructor.
  Tests the following:
  - it wraps a default-constructed (empty) container
*/
bool
tests_set_default_ctor()
{
    bool ok = true;

    test_kind_set<id_set> ks;
    ok = D_TK_CHECK(ks.size() == 0u) && ok;
    ok = D_TK_CHECK(ks.empty())      && ok;

    return ok;
}


/*
tests_set_copy_ctor
  Verifies the copy-from-container constructor.
  Tests the following:
  - the wrapper takes a copy of the supplied container's contents
  - the source container is left intact
*/
bool
tests_set_copy_ctor()
{
    bool ok = true;

    id_set src;
    src.data.push_back(1);
    src.data.push_back(2);

    test_kind_set<id_set> ks(src);
    ok = D_TK_CHECK(ks.size() == 2u)   && ok;
    ok = D_TK_CHECK(ks.contains(1))    && ok;
    ok = D_TK_CHECK(ks.contains(2))    && ok;
    ok = D_TK_CHECK(src.size() == 2u)  && ok;   // source untouched

    return ok;
}


/*
tests_set_move_ctor
  Verifies the move-from-container constructor.
  Tests the following:
  - the supplied container's contents are transferred into the wrapper
*/
bool
tests_set_move_ctor()
{
    bool ok = true;

    id_set src;
    src.data.push_back(3);
    src.data.push_back(4);
    src.data.push_back(5);

    test_kind_set<id_set> ks(static_cast<id_set&&>(src));
    ok = D_TK_CHECK(ks.size() == 3u) && ok;
    ok = D_TK_CHECK(ks.contains(3))  && ok;
    ok = D_TK_CHECK(ks.contains(5))  && ok;

    return ok;
}


/*
tests_set_underlying
  Verifies the underlying() accessors.
  Tests the following:
  - the mutable accessor exposes the container for direct edits
  - edits made through it are visible through the wrapper's surface
  - the const accessor names the same sub-object (address identity)
*/
bool
tests_set_underlying()
{
    bool ok = true;

    test_kind_set<id_set> ks;

    ks.underlying().data.push_back(8);
    ks.underlying().data.push_back(9);
    ok = D_TK_CHECK(ks.size() == 2u) && ok;
    ok = D_TK_CHECK(ks.contains(8))  && ok;

    const test_kind_set<id_set>& cks = ks;
    ok = D_TK_CHECK(cks.underlying().size() == 2u) && ok;
    ok = D_TK_CHECK(&ks.underlying() == &cks.underlying()) && ok;

    return ok;
}


/*
tests_set_ctor_noexcept
  Verifies the construction exception specifications and explicitness.
  Tests the following:
  - the default and move-from-container constructors are noexcept
  - the copy-from-container constructor is not noexcept
  - the container constructors are explicit (not implicitly convertible)
*/
bool
tests_set_ctor_noexcept()
{
    bool ok = true;

    ok = D_TK_CHECK(std::is_nothrow_default_constructible<
                        test_kind_set<id_set> >::value) && ok;
    ok = D_TK_CHECK(std::is_nothrow_constructible<
                        test_kind_set<id_set>, id_set&&>::value) && ok;
    ok = D_TK_CHECK(!std::is_nothrow_constructible<
                        test_kind_set<id_set>, const id_set&>::value) && ok;
    ok = D_TK_CHECK(!std::is_convertible<
                        const id_set&, test_kind_set<id_set> >::value) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
