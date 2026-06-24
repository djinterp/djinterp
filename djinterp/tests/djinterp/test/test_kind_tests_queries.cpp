// std
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   shared fixture                                                          ///
///////////////////////////////////////////////////////////////////////////////

namespace
{
    const test_option_set g_opts = { 42 };

    // sample_kinds
    //   a small range of records exercising leaf/interior, with/without
    //   options, an empty name, and a duplicate id (id 1 appears twice; the
    //   earlier, rank-5 leaf must win every lookup).
    std::vector<test_kind>
    sample_kinds()
    {
        std::vector<test_kind> v;
        v.push_back(make_test_kind(1,  "leaf_one",     5, true));
        v.push_back(make_test_kind(2,  "interior_two", 3, false, &g_opts));
        v.push_back(make_test_kind(10, "",             7, true));
        v.push_back(make_test_kind(1,  "dup_one",      9, false));  // dup id
        return v;
    }
}


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- IV. RESOLVED QUERIES                         ///
///////////////////////////////////////////////////////////////////////////////

// return types of the free functions over a range of records
static_assert(std::is_same<
    decltype(find_kind(std::declval<const std::vector<test_kind>&>(),
                       test_type_id(0))), const test_kind*>::value, "");
static_assert(std::is_same<
    decltype(rank_of(std::declval<const std::vector<test_kind>&>(),
                     test_type_id(0))), std::uint16_t>::value, "");
static_assert(std::is_same<
    decltype(is_leaf(std::declval<const std::vector<test_kind>&>(),
                     test_type_id(0))), bool>::value, "");
static_assert(std::is_same<
    decltype(is_interior(std::declval<const std::vector<test_kind>&>(),
                         test_type_id(0))), bool>::value, "");
static_assert(std::is_same<
    decltype(name_of(std::declval<const std::vector<test_kind>&>(),
                     test_type_id(0))), const char*>::value, "");
static_assert(std::is_same<
    decltype(default_options(std::declval<const std::vector<test_kind>&>(),
                             test_type_id(0))), const test_option_set*>::value,
    "");
static_assert(std::is_same<
    decltype(can_be_child_of(std::declval<const std::vector<test_kind>&>(),
                             test_type_id(0), test_type_id(0))), bool>::value,
    "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- IV. RESOLVED QUERIES                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_find_kind
  Verifies find_kind().
  Tests the following:
  - a registered id returns a pointer to its record
  - an unregistered id returns nullptr
  - on a duplicate id the first matching record wins
  - an empty range returns nullptr
*/
bool
tests_find_kind()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    const test_kind* p2 = find_kind(v, 2);
    ok = D_TK_CHECK(p2 != nullptr)  && ok;
    ok = D_TK_CHECK(p2->id == 2)    && ok;
    ok = D_TK_CHECK(p2->rank == 3)  && ok;

    ok = D_TK_CHECK(find_kind(v, 999) == nullptr) && ok;

    const test_kind* p1 = find_kind(v, 1);
    ok = D_TK_CHECK(p1 != nullptr)  && ok;
    ok = D_TK_CHECK(p1->rank == 5)  && ok;   // first match, not the rank-9 dup

    std::vector<test_kind> empty;
    ok = D_TK_CHECK(find_kind(empty, 1) == nullptr) && ok;

    return ok;
}


/*
tests_rank_of
  Verifies rank_of().
  Tests the following:
  - a registered id yields its kind's rank
  - an unregistered id falls back to the raw id cast to uint16
  - the fallback truncates ids beyond 16 bits and wraps negative ids
  - the first matching record governs a duplicate id
*/
bool
tests_rank_of()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    ok = D_TK_CHECK(rank_of(v, 2) == 3)  && ok;   // registered
    ok = D_TK_CHECK(rank_of(v, 1) == 5)  && ok;   // first match

    ok = D_TK_CHECK(rank_of(v, 999)
                        == static_cast<std::uint16_t>(999)) && ok;  // fallback
    ok = D_TK_CHECK(rank_of(v, 70000)
                        == static_cast<std::uint16_t>(70000)) && ok; // truncate
    ok = D_TK_CHECK(rank_of(v, -1)
                        == static_cast<std::uint16_t>(-1)) && ok;    // wrap

    std::vector<test_kind> empty;
    ok = D_TK_CHECK(rank_of(empty, 50)
                        == static_cast<std::uint16_t>(50)) && ok;

    return ok;
}


/*
tests_is_leaf
  Verifies is_leaf().
  Tests the following:
  - a registered leaf reports true and a registered interior reports false
  - an unregistered id defaults to leaf (true)
*/
bool
tests_is_leaf()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    ok = D_TK_CHECK(is_leaf(v, 1) == true)   && ok;   // leaf
    ok = D_TK_CHECK(is_leaf(v, 2) == false)  && ok;   // interior
    ok = D_TK_CHECK(is_leaf(v, 999) == true) && ok;   // unregistered default

    return ok;
}


/*
tests_is_interior
  Verifies is_interior().
  Tests the following:
  - it is the exact complement of is_leaf for registered ids
  - an unregistered id defaults to false (the complement of the leaf default)
*/
bool
tests_is_interior()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    ok = D_TK_CHECK(is_interior(v, 2) == true)    && ok;   // interior
    ok = D_TK_CHECK(is_interior(v, 1) == false)   && ok;   // leaf
    ok = D_TK_CHECK(is_interior(v, 999) == false) && ok;   // unregistered
    // complement relationship
    ok = D_TK_CHECK(is_interior(v, 2) == !is_leaf(v, 2)) && ok;
    ok = D_TK_CHECK(is_interior(v, 999) == !is_leaf(v, 999)) && ok;

    return ok;
}


/*
tests_name_of
  Verifies name_of().
  Tests the following:
  - a registered id returns its name
  - a registered empty name is returned as a valid empty string (not nullptr)
  - an unregistered id returns nullptr
*/
bool
tests_name_of()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    const char* n1 = name_of(v, 1);
    ok = D_TK_CHECK(n1 != nullptr)              && ok;
    ok = D_TK_CHECK(std::string(n1) == "leaf_one") && ok;

    const char* n10 = name_of(v, 10);
    ok = D_TK_CHECK(n10 != nullptr)     && ok;   // empty string, not null
    ok = D_TK_CHECK(n10[0] == '\0')     && ok;

    ok = D_TK_CHECK(name_of(v, 999) == nullptr) && ok;

    return ok;
}


/*
tests_default_options
  Verifies default_options().
  Tests the following:
  - a registered id with options returns that pointer
  - a registered id without options returns nullptr
  - an unregistered id returns nullptr
*/
bool
tests_default_options()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    ok = D_TK_CHECK(default_options(v, 2) == &g_opts)  && ok;  // has options
    ok = D_TK_CHECK(default_options(v, 1) == nullptr)  && ok;  // none set
    ok = D_TK_CHECK(default_options(v, 999) == nullptr) && ok; // unregistered

    return ok;
}


/*
tests_can_be_child_of
  Verifies can_be_child_of().
  Tests the following:
  - a lower-ranked child under a higher-ranked parent is allowed
  - the reverse is rejected; equal ranks are allowed
  - the rule holds when one or both ids fall back to their raw-id rank
*/
bool
tests_can_be_child_of()
{
    bool ok = true;

    std::vector<test_kind> v = sample_kinds();

    // registered: rank(2)=3, rank(1)=5
    ok = D_TK_CHECK(can_be_child_of(v, 2, 1) == true)  && ok;  // 3 <= 5
    ok = D_TK_CHECK(can_be_child_of(v, 1, 2) == false) && ok;  // 5 <= 3 false
    ok = D_TK_CHECK(can_be_child_of(v, 2, 2) == true)  && ok;  // equal

    // both unregistered -> raw-id ranks
    ok = D_TK_CHECK(can_be_child_of(v, 100, 200) == true)  && ok;
    ok = D_TK_CHECK(can_be_child_of(v, 200, 100) == false) && ok;

    // mixed: rank(2)=3 <= rank(100)=100
    ok = D_TK_CHECK(can_be_child_of(v, 2, 100) == true) && ok;

    return ok;
}


/*
tests_queries_compose
  Verifies that every query composes with a test_kind_set and behaves on an
  empty range.
  Tests the following:
  - over a populated test_kind_set<kind_set>, each query resolves the record
  - over an empty kind set, each query yields its documented fallback
*/
bool
tests_queries_compose()
{
    bool ok = true;

    test_kind_set<kind_set> ks;
    ks.insert(make_test_kind(7, "node7", 4, false, &g_opts));

    ok = D_TK_CHECK(find_kind(ks, 7) != nullptr)       && ok;
    ok = D_TK_CHECK(rank_of(ks, 7) == 4)               && ok;
    ok = D_TK_CHECK(is_leaf(ks, 7) == false)           && ok;
    ok = D_TK_CHECK(is_interior(ks, 7) == true)        && ok;
    ok = D_TK_CHECK(std::string(name_of(ks, 7)) == "node7") && ok;
    ok = D_TK_CHECK(default_options(ks, 7) == &g_opts) && ok;
    ok = D_TK_CHECK(can_be_child_of(ks, 7, 7) == true) && ok;

    // empty kind set -> fallbacks
    test_kind_set<kind_set> empty;
    ok = D_TK_CHECK(find_kind(empty, 1) == nullptr)          && ok;
    ok = D_TK_CHECK(rank_of(empty, 1) == static_cast<std::uint16_t>(1)) && ok;
    ok = D_TK_CHECK(is_leaf(empty, 1) == true)               && ok;
    ok = D_TK_CHECK(is_interior(empty, 1) == false)          && ok;
    ok = D_TK_CHECK(name_of(empty, 1) == nullptr)            && ok;
    ok = D_TK_CHECK(default_options(empty, 1) == nullptr)    && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
