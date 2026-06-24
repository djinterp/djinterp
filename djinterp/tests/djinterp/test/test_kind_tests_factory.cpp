// std
#include <cstdint>
#include <string>
#include <type_traits>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- II. FACTORY FUNCTION                         ///
///////////////////////////////////////////////////////////////////////////////

// the factory is a constant expression (single-return aggregate init)
namespace
{
    constexpr test_kind k_made      = make_test_kind(5, "made", 1, true);
    constexpr test_kind k_made_full = make_test_kind(6, "full", 2, false, nullptr);
}
static_assert(k_made.id == 5,                  "");
static_assert(k_made.rank == 1,                "");
static_assert(k_made.is_leaf == true,          "");
static_assert(k_made.default_options == nullptr,
              "omitted default_options must default to nullptr");
static_assert(k_made_full.is_leaf == false,    "");

// noexcept and return type
static_assert(noexcept(make_test_kind(0, "", 0, false)), "");
static_assert(noexcept(make_test_kind(0, "", 0, false, nullptr)), "");
static_assert(std::is_same<decltype(make_test_kind(0, "", 0, false)),
                           test_kind>::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- II. FACTORY FUNCTION                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_make_test_kind_basic
  Verifies the full five-argument factory.
  Tests the following:
  - every argument lands in the matching field
  - the result equals the equivalent aggregate-initialized record
  - the full value range is accepted (extremes, nullptr name, real options ptr)
*/
bool
tests_make_test_kind_basic()
{
    bool ok = true;

    const test_option_set opts = { 3 };

    test_kind k = make_test_kind(10, "kind", 2, true, &opts);
    ok = D_TK_CHECK(k.id == 10)                    && ok;
    ok = D_TK_CHECK(std::string(k.name) == "kind") && ok;
    ok = D_TK_CHECK(k.rank == 2)                   && ok;
    ok = D_TK_CHECK(k.is_leaf == true)             && ok;
    ok = D_TK_CHECK(k.default_options == &opts)    && ok;

    // matches the equivalent aggregate
    test_kind agg = { 10, "kind", 2, true, &opts };
    ok = D_TK_CHECK(k.id == agg.id)             && ok;
    ok = D_TK_CHECK(k.rank == agg.rank)         && ok;
    ok = D_TK_CHECK(k.is_leaf == agg.is_leaf)   && ok;
    ok = D_TK_CHECK(k.name == agg.name)         && ok;  // same literal address
    ok = D_TK_CHECK(k.default_options == agg.default_options) && ok;

    // extremes
    test_kind lo = make_test_kind(INT32_MIN, nullptr, 0, false);
    ok = D_TK_CHECK(lo.id == INT32_MIN)  && ok;
    ok = D_TK_CHECK(lo.name == nullptr)  && ok;
    test_kind hi = make_test_kind(INT32_MAX, "x", UINT16_MAX, true, &opts);
    ok = D_TK_CHECK(hi.id == INT32_MAX)      && ok;
    ok = D_TK_CHECK(hi.rank == UINT16_MAX)   && ok;

    return ok;
}


/*
tests_make_test_kind_default_arg
  Verifies the four-argument form.
  Tests the following:
  - omitting default_options leaves it nullptr
  - the other four fields are unaffected by the defaulting
*/
bool
tests_make_test_kind_default_arg()
{
    bool ok = true;

    test_kind k = make_test_kind(20, "noopts", 7, false);
    ok = D_TK_CHECK(k.default_options == nullptr) && ok;
    ok = D_TK_CHECK(k.id == 20)                   && ok;
    ok = D_TK_CHECK(k.rank == 7)                  && ok;
    ok = D_TK_CHECK(k.is_leaf == false)           && ok;

    return ok;
}


/*
tests_make_test_kind_constexpr
  Verifies the factory is usable in a constant expression.
  Tests the following:
  - a constexpr record can be built from it and its fields read at compile time
  - the same call returns the expected values at runtime
*/
bool
tests_make_test_kind_constexpr()
{
    bool ok = true;

    D_CONSTEXPR test_kind k = make_test_kind(30, "ce", 9, true);
    ok = D_TK_CHECK(k.id == 30)   && ok;
    ok = D_TK_CHECK(k.rank == 9)  && ok;
    ok = D_TK_CHECK(k.is_leaf)    && ok;

    return ok;
}


/*
tests_make_test_kind_noexcept
  Verifies the factory's exception specification.
  Tests the following:
  - both the four- and five-argument forms are noexcept
*/
bool
tests_make_test_kind_noexcept()
{
    bool ok = true;

    ok = D_TK_CHECK(noexcept(make_test_kind(0, "", 0, false)))          && ok;
    ok = D_TK_CHECK(noexcept(make_test_kind(0, "", 0, false, nullptr))) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
