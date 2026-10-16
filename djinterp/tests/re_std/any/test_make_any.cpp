/*******************************************************************************
* djinterp [test]                                              test_make_any.cpp
*
* re_std::make_any tests:
*   make_any<T>(args...) and make_any<T>(initializer_list, args...).
* Available only on C++11+ (requires variadic templates).
*
*
* path:      /tests/djinterp/re_std/any/test_make_any.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "restd_any_tests.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

// std
#include <vector>

using re_std::any;
using re_std::make_any;
namespace tests = restd_any_tests;


// ===========================================================================
// MAKE_ANY - SBO TYPES
// ===========================================================================

TEST_CASE(make_any, sbo_int)
{
    any a = make_any<int>(42);

    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), 42);
}


TEST_CASE(make_any, sbo_double)
{
    any a = make_any<double>(3.14);

    EXPECT_TRUE(a.holds<double>());
    EXPECT_EQ(a.get<double>(), 3.14);
}


TEST_CASE(make_any, sbo_pointer)
{
    int  x = 7;
    any  a = make_any<int*>(&x);

    EXPECT_TRUE(a.holds<int*>());
    EXPECT_EQ(a.get<int*>(), &x);
}


// ===========================================================================
// MAKE_ANY - HEAP TYPES
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(make_any, heap_string_default)
{
    any a = make_any<std::string>();

    EXPECT_TRUE(a.holds<std::string>());
    EXPECT_EQ(a.get_ref<std::string>(), std::string(""));
}


TEST_CASE(make_any, heap_string_from_literal)
{
    any a = make_any<std::string>("constructed");

    EXPECT_EQ(a.get_ref<std::string>(), std::string("constructed"));
}


TEST_CASE(make_any, heap_string_with_args)
{
    any a = make_any<std::string>(static_cast<std::size_t>(4), 'q');

    EXPECT_EQ(a.get_ref<std::string>(), std::string("qqqq"));
}


TEST_CASE(make_any, heap_aggregate)
{
    any a = make_any<tests::Aggregate>(5, 6.5);

    const tests::Aggregate& ag = a.get_ref<tests::Aggregate>();
    EXPECT_EQ(ag.m_a, 5);
    EXPECT_EQ(ag.m_b, 6.5);
}


TEST_CASE(make_any, heap_vector_initializer_list)
{
    any a = make_any<std::vector<int> >({10, 20, 30});

    std::vector<int>& v = a.get_ref<std::vector<int> >();
    EXPECT_EQ(v.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
}


TEST_CASE(make_any, heap_no_unnecessary_copies)
{
    tests::Counted::reset_counts();

    {
        any a = make_any<tests::Counted>(99);
        EXPECT_TRUE(tests::Counted::alive() > 0);
        // emplace construction should keep alive count low
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif  // D_ENV_CPP98_HAS_NEW


// ===========================================================================
// MAKE_ANY - PERFECT FORWARDING
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
namespace
{

struct CountForward
{
    int m_lvalue_ctor;
    int m_rvalue_ctor;

    CountForward(const std::string& _s)
        : m_lvalue_ctor(1),
          m_rvalue_ctor(0)
    {
        (void)_s;
    }

    CountForward(std::string&& _s)
        : m_lvalue_ctor(0),
          m_rvalue_ctor(1)
    {
        (void)_s;
    }
};

}  // namespace


TEST_CASE(make_any, forwards_rvalue_correctly)
{
    any a = re_std::make_any<CountForward>(std::string("rvalue"));

    const CountForward& cf = a.get_ref<CountForward>();
    EXPECT_EQ(cf.m_rvalue_ctor, 1);
    EXPECT_EQ(cf.m_lvalue_ctor, 0);
}


TEST_CASE(make_any, forwards_lvalue_correctly)
{
    std::string s("lvalue");
    any         a = re_std::make_any<CountForward>(s);

    const CountForward& cf = a.get_ref<CountForward>();
    EXPECT_EQ(cf.m_lvalue_ctor, 1);
    EXPECT_EQ(cf.m_rvalue_ctor, 0);
}
#endif

#else  // no variadic templates

// Suite placeholder so the runner reports "0 tests in suite make_any"
// instead of "no such suite".
TEST_CASE(make_any, unavailable_on_cpp98)
{
    EXPECT_TRUE(true);
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
