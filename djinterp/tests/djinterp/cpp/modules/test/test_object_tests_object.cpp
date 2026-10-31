/*******************************************************************************
* djinterp [cpp/test]                               test_object_tests_object.cpp
*
*
* path:      /tests/djinterp/cpp/modules/test/test_object_tests_object.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


// status-type variety used to confirm the template accepts any arithmetic
// status code and lays the five status constants out correctly
namespace
{
    using DU16Object = DTestObject<std::uint16_t, DLitmeta>;
    using DI32Object = DTestObject<std::int32_t,  DLitmeta>;
    using DI64Object = DTestObject<std::int64_t,  DLitmeta>;

    // extra _Options tail entries must be inert
    using DOptObject = DTestObject<std::uint8_t, DLitmeta, int, char, void>;
}  // namespace


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- aliases / status constants / storage / traits ///
///////////////////////////////////////////////////////////////////////////////

// -- type aliases -------------------------------------------------------------
static_assert(std::is_same<basic_test::status_type, std::uint8_t>::value, "");
static_assert(std::is_same<basic_test::metadata_container_type,
                           DTestMetadata>::value, "");
static_assert(std::is_same<basic_test::metadata_type,
                  ::djinterp::DKvPair<std::string, std::string> >::value, "");

static_assert(std::is_same<DTaggedTest::metadata_container_type,
                           std::vector<std::int32_t> >::value, "");
static_assert(std::is_same<DTaggedTest::metadata_type, std::int32_t>::value, "");

// -- status constants : values are fixed by contract --------------------------
static_assert(static_cast<int>(basic_test::status_passed)  == 0, "");
static_assert(static_cast<int>(basic_test::status_failed)  == 1, "");
static_assert(static_cast<int>(basic_test::status_skipped) == 2, "");
static_assert(static_cast<int>(basic_test::status_pending) == 3, "");
static_assert(static_cast<int>(basic_test::status_error)   == 4, "");

// -- status constants : typed as the chosen status_type -----------------------
static_assert(std::is_same<std::remove_cv<
                  decltype(basic_test::status_passed)>::type,
                  std::uint8_t>::value, "");
static_assert(std::is_same<std::remove_cv<
                  decltype(DI64Object::status_error)>::type,
                  std::int64_t>::value, "");

// -- storage : member types ---------------------------------------------------
//   ruling 13 removed the stored m_result; the verdict is derived, so what is
// pinned here is the accessor's return type, not a member's.
static_assert(std::is_same<decltype(std::declval<const basic_test&>().result()),
                           bool>::value, "");
static_assert(std::is_same<decltype(basic_test::m_status),
                           basic_test::status_type>::value, "");
static_assert(std::is_same<decltype(basic_test::m_type_id),
                           DTestTypeId>::value, "");
static_assert(std::is_same<decltype(basic_test::m_callable_id),
                           DTestCallableId>::value, "");
static_assert(std::is_same<decltype(basic_test::m_metadata),
                           DTestMetadata>::value, "");
static_assert(std::is_same<decltype(DLitObject::m_metadata), DLitmeta>::value, "");

// -- traits : triviality tracks the metadata container ------------------------
static_assert(!std::is_trivially_copyable<basic_test>::value, "");
static_assert(std::is_trivially_copyable<DLitObject>::value, "");
static_assert(!std::is_trivially_destructible<basic_test>::value, "");
static_assert(std::is_trivially_destructible<DLitObject>::value, "");
// a user-declared default ctor is present, so default construction is never
// trivial regardless of the container
static_assert(!std::is_trivially_default_constructible<basic_test>::value, "");
static_assert(!std::is_trivially_default_constructible<DLitObject>::value, "");
// litmeta is a literal type, so the whole object is standard-layout (portable;
// the vector-backed instantiations' layout is implementation-defined)
static_assert(std::is_standard_layout<DLitObject>::value, "");

// -- traits : the type_id constructor is explicit -----------------------------
static_assert(std::is_constructible<basic_test, DTestTypeId>::value, "");
static_assert(!std::is_convertible<DTestTypeId, basic_test>::value, "");

// -- traits : the trailing _Options pack is inert -----------------------------
static_assert(std::is_same<DOptObject::status_type, std::uint8_t>::value, "");
static_assert(std::is_same<DOptObject::metadata_container_type,
                           DLitmeta>::value, "");
static_assert(std::is_trivially_copyable<DOptObject>::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- aliases / status constants / storage / traits          ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_object_aliases
  Verifies the metadata-protocol and status type aliases.
  Tests the following:
  - basic_test exposes uint8 status, DTestMetadata container, DKvPair row
  - DTaggedTest exposes a vector<int32> container with int32 row
  - DLitObject threads its custom container and that container's value_type
*/
bool tests_object_aliases()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_same<basic_test::status_type,
                                 std::uint8_t>::value) && ok;
    ok = D_TO_CHECK(std::is_same<basic_test::metadata_container_type,
                                 DTestMetadata>::value) && ok;
    ok = D_TO_CHECK(std::is_same<basic_test::metadata_type,
                    ::djinterp::DKvPair<std::string, std::string> >::value) && ok;

    ok = D_TO_CHECK(std::is_same<DTaggedTest::metadata_container_type,
                                 std::vector<std::int32_t> >::value) && ok;
    ok = D_TO_CHECK(std::is_same<DTaggedTest::metadata_type,
                                 std::int32_t>::value) && ok;

    ok = D_TO_CHECK(std::is_same<DLitObject::metadata_container_type,
                                 DLitmeta>::value) && ok;
    ok = D_TO_CHECK(std::is_same<DLitObject::metadata_type, int>::value) && ok;

    return ok;
}


/*
tests_object_status_constants
  Verifies the five status constants.
  Tests the following:
  - each carries its contracted integer value (0..4)
  - all five are mutually distinct
  - the values survive in a wider status type without collision
*/
bool tests_object_status_constants()
{
    bool ok = true;

    ok = D_TO_CHECK(static_cast<int>(basic_test::status_passed)  == 0) && ok;
    ok = D_TO_CHECK(static_cast<int>(basic_test::status_failed)  == 1) && ok;
    ok = D_TO_CHECK(static_cast<int>(basic_test::status_skipped) == 2) && ok;
    ok = D_TO_CHECK(static_cast<int>(basic_test::status_pending) == 3) && ok;
    ok = D_TO_CHECK(static_cast<int>(basic_test::status_error)   == 4) && ok;

    // distinctness (uint8 holds all five without aliasing)
    const int v[] = {
        static_cast<int>(basic_test::status_passed),
        static_cast<int>(basic_test::status_failed),
        static_cast<int>(basic_test::status_skipped),
        static_cast<int>(basic_test::status_pending),
        static_cast<int>(basic_test::status_error)
    };
    for (std::size_t i = 0; i < 5; ++i)
    {
        for (std::size_t j = i + 1; j < 5; ++j)
        {
            ok = D_TO_CHECK(v[i] != v[j]) && ok;
        }
    }

    // a wider status type keeps the same five distinct values
    ok = D_TO_CHECK(static_cast<int>(DU16Object::status_error)   == 4) && ok;
    ok = D_TO_CHECK(static_cast<int>(DI32Object::status_pending) == 3) && ok;
    ok = D_TO_CHECK(static_cast<long long>(DI64Object::status_skipped) == 2)
         && ok;

    return ok;
}


/*
tests_object_storage
  Verifies the stored members' types.
  Tests the following:
  - m_status / m_type_id / m_callable_id / m_metadata have the declared types
  - result() is derived from m_status and still returns bool (ruling 13)
  - a custom container threads straight through to m_metadata
  (No unique-id or depth member exists: identity and depth are the owning
  tree's concern, per the header contract.)
*/
bool tests_object_storage()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_same<
                        decltype(std::declval<const basic_test&>().result()),
                        bool>::value) && ok;
    ok = D_TO_CHECK(std::is_same<decltype(basic_test::m_status),
                                 basic_test::status_type>::value) && ok;
    ok = D_TO_CHECK(std::is_same<decltype(basic_test::m_type_id),
                                 DTestTypeId>::value) && ok;
    ok = D_TO_CHECK(std::is_same<decltype(basic_test::m_callable_id),
                                 DTestCallableId>::value) && ok;
    ok = D_TO_CHECK(std::is_same<decltype(basic_test::m_metadata),
                                 DTestMetadata>::value) && ok;
    ok = D_TO_CHECK(std::is_same<decltype(DLitObject::m_metadata),
                                 DLitmeta>::value) && ok;

    return ok;
}


/*
tests_object_traits
  Verifies the structural type traits.
  Tests the following:
  - trivial copyability and trivial destructibility track the metadata
    container (literal container -> trivial; vector-backed -> not)
  - default construction is never trivial (a user-declared default ctor exists)
  - the type_id constructor is explicit (constructible, not convertible)
  - a literal-backed object is standard-layout
  - extra trailing _Options leave the type unchanged
*/
bool tests_object_traits()
{
    bool ok = true;

    ok = D_TO_CHECK(!std::is_trivially_copyable<basic_test>::value)     && ok;
    ok = D_TO_CHECK(std::is_trivially_copyable<DLitObject>::value)      && ok;
    ok = D_TO_CHECK(!std::is_trivially_destructible<basic_test>::value) && ok;
    ok = D_TO_CHECK(std::is_trivially_destructible<DLitObject>::value)  && ok;

    ok = D_TO_CHECK(!std::is_trivially_default_constructible<
                        basic_test>::value) && ok;
    ok = D_TO_CHECK(std::is_default_constructible<basic_test>::value) && ok;

    ok = D_TO_CHECK(std::is_constructible<basic_test, DTestTypeId>::value)
         && ok;
    ok = D_TO_CHECK(!std::is_convertible<DTestTypeId, basic_test>::value)
         && ok;

    ok = D_TO_CHECK(std::is_standard_layout<DLitObject>::value) && ok;

    // _Options inert: same observable type-level behaviour as without them
    ok = D_TO_CHECK(std::is_same<DOptObject::status_type,
                                 std::uint8_t>::value) && ok;
    ok = D_TO_CHECK(std::is_trivially_copyable<DOptObject>::value) && ok;
    {
        DOptObject t(5, true);
        ok = D_TO_CHECK(t.type_id() == 5)  && ok;
        ok = D_TO_CHECK(t.passed())        && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
