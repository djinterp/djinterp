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
///   compile-time invariants -- I. TEST KIND RECORD                          ///
///////////////////////////////////////////////////////////////////////////////

// member types
static_assert(std::is_same<decltype(test_kind::id), test_type_id>::value, "");
static_assert(std::is_same<decltype(test_kind::name), const char*>::value, "");
static_assert(std::is_same<decltype(test_kind::rank), std::uint16_t>::value, "");
static_assert(std::is_same<decltype(test_kind::is_leaf), bool>::value, "");
static_assert(std::is_same<decltype(test_kind::default_options),
                           const test_option_set*>::value, "");

// structural traits: a plain aggregate value type
static_assert(std::is_standard_layout<test_kind>::value, "");
static_assert(std::is_trivially_copyable<test_kind>::value, "");
static_assert(std::is_trivially_destructible<test_kind>::value, "");
static_assert(std::is_default_constructible<test_kind>::value, "");
static_assert(std::is_trivially_default_constructible<test_kind>::value, "");
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
static_assert(std::is_aggregate<test_kind>::value, "");
#endif

// aggregate initialization is a constant expression
namespace
{
    constexpr test_kind k_ce = { 10, "ce", 2, true, nullptr };
}
static_assert(k_ce.id == 10,        "");
static_assert(k_ce.rank == 2,       "");
static_assert(k_ce.is_leaf == true, "");
static_assert(k_ce.default_options == nullptr, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- I. TEST KIND RECORD                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_test_kind_aggregate
  Verifies aggregate initialization of the record.
  Tests the following:
  - positional brace/equals initialization stores each field in order
    (id, name, rank, is_leaf, default_options)
  - a pointer to an option set is retained verbatim
  - C++20 designated initialization, where available, agrees field-for-field
*/
bool
tests_test_kind_aggregate()
{
    bool ok = true;

    const test_option_set opts = { 7 };

    test_kind k = { 10, "my_kind", 2, true, &opts };
    ok = D_TK_CHECK(k.id == 10)                       && ok;
    ok = D_TK_CHECK(std::string(k.name) == "my_kind") && ok;
    ok = D_TK_CHECK(k.rank == 2)                      && ok;
    ok = D_TK_CHECK(k.is_leaf == true)                && ok;
    ok = D_TK_CHECK(k.default_options == &opts)       && ok;

    // brace-init without '=' agrees
    test_kind k2{ 11, "other", 3, false, nullptr };
    ok = D_TK_CHECK(k2.id == 11)               && ok;
    ok = D_TK_CHECK(k2.is_leaf == false)       && ok;
    ok = D_TK_CHECK(k2.default_options == nullptr) && ok;

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    test_kind k3 = { .id = 12, .name = "desig", .rank = 4,
                     .is_leaf = true, .default_options = nullptr };
    ok = D_TK_CHECK(k3.id == 12)   && ok;
    ok = D_TK_CHECK(k3.rank == 4)  && ok;
#endif

    return ok;
}


/*
tests_test_kind_members
  Verifies the record's member types.
  Tests the following:
  - id is test_type_id, name is const char*, rank is uint16,
    is_leaf is bool, default_options is const test_option_set*
*/
bool
tests_test_kind_members()
{
    bool ok = true;

    ok = D_TK_CHECK(std::is_same<decltype(test_kind::id),
                                 test_type_id>::value) && ok;
    ok = D_TK_CHECK(std::is_same<decltype(test_kind::name),
                                 const char*>::value) && ok;
    ok = D_TK_CHECK(std::is_same<decltype(test_kind::rank),
                                 std::uint16_t>::value) && ok;
    ok = D_TK_CHECK(std::is_same<decltype(test_kind::is_leaf),
                                 bool>::value) && ok;
    ok = D_TK_CHECK(std::is_same<decltype(test_kind::default_options),
                                 const test_option_set*>::value) && ok;

    return ok;
}


/*
tests_test_kind_traits
  Verifies the record's structural type traits.
  Tests the following:
  - standard-layout, trivially copyable, trivially destructible
  - default-constructible and trivially so (a plain aggregate of scalars and
    pointers, with no user-declared constructors)
*/
bool
tests_test_kind_traits()
{
    bool ok = true;

    ok = D_TK_CHECK(std::is_standard_layout<test_kind>::value)            && ok;
    ok = D_TK_CHECK(std::is_trivially_copyable<test_kind>::value)         && ok;
    ok = D_TK_CHECK(std::is_trivially_destructible<test_kind>::value)     && ok;
    ok = D_TK_CHECK(std::is_default_constructible<test_kind>::value)      && ok;
    ok = D_TK_CHECK(std::is_trivially_default_constructible<
                        test_kind>::value) && ok;

    return ok;
}


/*
tests_test_kind_values
  Verifies the record across the full range of field values.
  Tests the following:
  - id spans the signed range (negative, zero, extremes)
  - rank spans the unsigned 16-bit range (0 .. 65535)
  - is_leaf takes both truth values
  - name accepts nullptr, the empty string, and a normal literal
  - default_options accepts nullptr and a real pointer
*/
bool
tests_test_kind_values()
{
    bool ok = true;

    const test_option_set opts = { 99 };

    test_kind lo = { INT32_MIN, nullptr, 0, false, nullptr };
    ok = D_TK_CHECK(lo.id == INT32_MIN)         && ok;
    ok = D_TK_CHECK(lo.rank == 0)               && ok;
    ok = D_TK_CHECK(lo.name == nullptr)         && ok;
    ok = D_TK_CHECK(lo.is_leaf == false)        && ok;

    test_kind hi = { INT32_MAX, "", UINT16_MAX, true, &opts };
    ok = D_TK_CHECK(hi.id == INT32_MAX)         && ok;
    ok = D_TK_CHECK(hi.rank == UINT16_MAX)      && ok;
    ok = D_TK_CHECK(hi.name != nullptr)         && ok;
    ok = D_TK_CHECK(hi.name[0] == '\0')         && ok;
    ok = D_TK_CHECK(hi.default_options == &opts) && ok;

    test_kind zero = { 0, "z", 1, true, nullptr };
    ok = D_TK_CHECK(zero.id == 0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
