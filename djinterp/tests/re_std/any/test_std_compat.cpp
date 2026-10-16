/*******************************************************************************
* djinterp [test]                                            test_std_compat.cpp
*
* re_std::any vs std::any compatibility tests:
*   For every operation, verify that re_std::any and std::any behave
* identically given the same inputs.  These tests run only on
* C++17+ where <any> is available.
*
* COVERAGE:
*   - construction parity
*   - has_value parity
*   - reset parity
*   - swap parity
*   - emplace parity
*   - any_cast parity (pointer + reference overloads)
*   - bad_any_cast vs std::bad_any_cast (parallel throwing behaviour)
*   - copy / move semantics parity
*
*
* path:      /tests/djinterp/re_std/any/test_std_compat.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "restd_any_tests.hpp"


#if RESTD_ANY_TESTS_HAS_STD_ANY

// std
#include <vector>

using re_std::any;
namespace tests = restd_any_tests;


// ===========================================================================
// CONSTRUCTION PARITY
// ===========================================================================

TEST_CASE(std_compat, default_construct_parity)
{
    any      r;
    std::any s;

    EXPECT_EQ(r.has_value(), s.has_value());
    EXPECT_FALSE(r.has_value());
}


TEST_CASE(std_compat, int_construct_parity)
{
    any      r(42);
    std::any s(42);

    EXPECT_EQ(r.has_value(), s.has_value());
    EXPECT_EQ(re_std::any_cast<int>(r),
              std::any_cast<int>(s));
}


TEST_CASE(std_compat, string_construct_parity)
{
    std::string val("hello");
    any         r(val);
    std::any    s(val);

    EXPECT_EQ(r.has_value(), s.has_value());
    EXPECT_EQ(re_std::any_cast<std::string>(r),
              std::any_cast<std::string>(s));
}


TEST_CASE(std_compat, pointer_construct_parity)
{
    int x = 99;
    any      r(&x);
    std::any s(&x);

    EXPECT_EQ(re_std::any_cast<int*>(r),
              std::any_cast<int*>(s));
}


// ===========================================================================
// COPY / MOVE PARITY
// ===========================================================================

TEST_CASE(std_compat, copy_construct_parity)
{
    any      r1(42);
    std::any s1(42);

    any      r2(r1);
    std::any s2(s1);

    EXPECT_EQ(re_std::any_cast<int>(r1), std::any_cast<int>(s1));
    EXPECT_EQ(re_std::any_cast<int>(r2), std::any_cast<int>(s2));

    // both originals retained
    EXPECT_TRUE(r1.has_value());
    EXPECT_TRUE(s1.has_value());
}


TEST_CASE(std_compat, move_construct_parity)
{
    any      r1(std::string("moved"));
    std::any s1(std::string("moved"));

    any      r2(std::move(r1));
    std::any s2(std::move(s1));

    EXPECT_EQ(r1.has_value(), s1.has_value());
    EXPECT_TRUE(r2.has_value());
    EXPECT_TRUE(s2.has_value());
}


TEST_CASE(std_compat, copy_assign_parity)
{
    any      r1(7);
    std::any s1(7);
    any      r2;
    std::any s2;

    r2 = r1;
    s2 = s1;

    EXPECT_EQ(re_std::any_cast<int>(r2), std::any_cast<int>(s2));
}


// ===========================================================================
// RESET PARITY
// ===========================================================================

TEST_CASE(std_compat, reset_parity)
{
    any      r(1);
    std::any s(1);

    r.reset();
    s.reset();

    EXPECT_EQ(r.has_value(), s.has_value());
    EXPECT_FALSE(r.has_value());
}


// ===========================================================================
// SWAP PARITY
// ===========================================================================

TEST_CASE(std_compat, member_swap_parity)
{
    any      r1(1);
    any      r2(2);
    std::any s1(1);
    std::any s2(2);

    r1.swap(r2);
    s1.swap(s2);

    EXPECT_EQ(re_std::any_cast<int>(r1), std::any_cast<int>(s1));
    EXPECT_EQ(re_std::any_cast<int>(r2), std::any_cast<int>(s2));
}


TEST_CASE(std_compat, free_swap_parity)
{
    any      r1(std::string("a"));
    any      r2(std::string("b"));
    std::any s1(std::string("a"));
    std::any s2(std::string("b"));

    re_std::swap(r1, r2);
    std::swap(s1, s2);

    EXPECT_EQ(re_std::any_cast<std::string>(r1),
              std::any_cast<std::string>(s1));
    EXPECT_EQ(re_std::any_cast<std::string>(r2),
              std::any_cast<std::string>(s2));
}


// ===========================================================================
// EMPLACE PARITY
// ===========================================================================

TEST_CASE(std_compat, emplace_sbo_parity)
{
    any      r;
    std::any s;

    r.emplace<int>(99);
    s.emplace<int>(99);

    EXPECT_EQ(re_std::any_cast<int>(r), std::any_cast<int>(s));
}


TEST_CASE(std_compat, emplace_heap_parity)
{
    any      r;
    std::any s;

    r.emplace<std::string>(5u, 'q');
    s.emplace<std::string>(5u, 'q');

    EXPECT_EQ(re_std::any_cast<std::string>(r),
              std::any_cast<std::string>(s));
}


TEST_CASE(std_compat, emplace_initializer_list_parity)
{
    any      r;
    std::any s;

    r.emplace<std::vector<int>>({1, 2, 3});
    s.emplace<std::vector<int>>({1, 2, 3});

    const auto& rv = r.get_ref<std::vector<int>>();
    const auto& sv = std::any_cast<const std::vector<int>&>(s);

    EXPECT_EQ(rv.size(), sv.size());
    for (std::size_t i = 0; i < rv.size(); ++i)
    {
        EXPECT_EQ(rv[i], sv[i]);
    }
}


// ===========================================================================
// ANY_CAST PARITY - POINTER OVERLOADS
// ===========================================================================

TEST_CASE(std_compat, any_cast_pointer_match_parity)
{
    any      r(std::string("x"));
    std::any s(std::string("x"));

    auto* rp = re_std::any_cast<std::string>(&r);
    auto* sp = std::any_cast<std::string>(&s);

    EXPECT_TRUE(rp != nullptr);
    EXPECT_TRUE(sp != nullptr);
    EXPECT_EQ(*rp, *sp);
}


TEST_CASE(std_compat, any_cast_pointer_mismatch_parity)
{
    any      r(std::string("x"));
    std::any s(std::string("x"));

    auto* rp = re_std::any_cast<int>(&r);
    auto* sp = std::any_cast<int>(&s);

    EXPECT_EQ(rp, static_cast<int*>(nullptr));
    EXPECT_EQ(sp, static_cast<int*>(nullptr));
}


// ===========================================================================
// ANY_CAST PARITY - REFERENCE OVERLOADS / EXCEPTIONS
// ===========================================================================

TEST_CASE(std_compat, any_cast_ref_match_parity)
{
    any      r(42);
    std::any s(42);

    int rv = re_std::any_cast<int>(static_cast<const any&>(r));
    int sv = std::any_cast<int>(static_cast<const std::any&>(s));

    EXPECT_EQ(rv, sv);
}


TEST_CASE(std_compat, any_cast_ref_mismatch_throws_parity)
{
    any      r(42);
    std::any s(42);

    // both should throw their respective bad_*_cast
    bool r_threw = false;
    bool s_threw = false;

    try
    {
        re_std::any_cast<double>(static_cast<const any&>(r));
    }
    catch (const re_std::bad_any_cast&)
    {
        r_threw = true;
    }

    try
    {
        std::any_cast<double>(static_cast<const std::any&>(s));
    }
    catch (const std::bad_any_cast&)
    {
        s_threw = true;
    }

    EXPECT_TRUE(r_threw);
    EXPECT_TRUE(s_threw);
    EXPECT_EQ(r_threw, s_threw);
}


TEST_CASE(std_compat, any_cast_empty_throws_parity)
{
    any      r;
    std::any s;

    bool r_threw = false;
    bool s_threw = false;

    try { re_std::any_cast<int>(static_cast<const any&>(r)); }
    catch (const re_std::bad_any_cast&) { r_threw = true; }

    try { std::any_cast<int>(static_cast<const std::any&>(s)); }
    catch (const std::bad_any_cast&) { s_threw = true; }

    EXPECT_TRUE(r_threw);
    EXPECT_TRUE(s_threw);
}


// ===========================================================================
// MAKE_ANY PARITY
// ===========================================================================

TEST_CASE(std_compat, make_any_int_parity)
{
    any      r = re_std::make_any<int>(7);
    std::any s = std::make_any<int>(7);

    EXPECT_EQ(re_std::any_cast<int>(r), std::any_cast<int>(s));
}


TEST_CASE(std_compat, make_any_string_parity)
{
    any      r = re_std::make_any<std::string>("forwarded");
    std::any s = std::make_any<std::string>("forwarded");

    EXPECT_EQ(re_std::any_cast<std::string>(r),
              std::any_cast<std::string>(s));
}


TEST_CASE(std_compat, make_any_args_parity)
{
    any      r = re_std::make_any<std::string>(3u, 'z');
    std::any s = std::make_any<std::string>(3u, 'z');

    EXPECT_EQ(re_std::any_cast<std::string>(r),
              std::any_cast<std::string>(s));
}


// ===========================================================================
// LIFETIME PARITY
// ===========================================================================

TEST_CASE(std_compat, destruction_parity_no_leaks)
{
    tests::Counted::reset_counts();
    int restd_alive_at_scope_end = -1;

    {
        any      r(tests::Counted(1));
        std::any s(tests::Counted(2));
        (void)r;
        (void)s;
    }

    restd_alive_at_scope_end = tests::Counted::alive();

    EXPECT_EQ(restd_alive_at_scope_end, 0);
}


#else  // !RESTD_ANY_TESTS_HAS_STD_ANY

#include "restd_any_tests.hpp"


// std::any unavailable - emit a single placeholder test
TEST_CASE(std_compat, std_any_unavailable)
{
    EXPECT_TRUE(true);
}


#endif  // RESTD_ANY_TESTS_HAS_STD_ANY
