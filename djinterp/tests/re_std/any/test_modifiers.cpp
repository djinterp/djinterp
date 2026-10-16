/*******************************************************************************
* djinterp [test]                                             test_modifiers.cpp
*
* re_std::any modifier tests:
*   reset(), swap(), emplace<T>(...), copy / move assignment.
*
*
* path:      /tests/djinterp/re_std/any/test_modifiers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "restd_any_tests.hpp"


using re_std::any;
using re_std::DAnyCategory;
namespace tests = restd_any_tests;


// ===========================================================================
// RESET
// ===========================================================================

TEST_CASE(modifiers, reset_empty_is_noop)
{
    any a;
    a.reset();
    EXPECT_FALSE(a.has_value());
}


TEST_CASE(modifiers, reset_sbo)
{
    any a(42);
    EXPECT_TRUE(a.has_value());

    a.reset();
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.category(), DAnyCategory::cat_empty);
    EXPECT_EQ(a.type(), static_cast<re_std::any_type_id>(0));
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, reset_heap_destroys)
{
    tests::Counted::reset_counts();

    any a(tests::Counted(7));
    EXPECT_TRUE(tests::Counted::alive() > 0);

    a.reset();
    EXPECT_EQ(tests::Counted::alive(), 0);
    EXPECT_FALSE(a.has_value());
}


TEST_CASE(modifiers, reset_then_reuse)
{
    any a(std::string("first"));
    a.reset();
    a = 42;

    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), 42);
}
#endif


// ===========================================================================
// SWAP - MEMBER
// ===========================================================================

TEST_CASE(modifiers, swap_sbo_sbo)
{
    any a(1);
    any b(2);

    a.swap(b);

    EXPECT_EQ(a.get<int>(), 2);
    EXPECT_EQ(b.get<int>(), 1);
}


TEST_CASE(modifiers, swap_empty_sbo)
{
    any a;
    any b(7);

    a.swap(b);

    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(b.has_value());
    EXPECT_EQ(a.get<int>(), 7);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, swap_sbo_heap)
{
    any a(99);
    any b(std::string("text"));

    a.swap(b);

    EXPECT_TRUE(a.holds<std::string>());
    EXPECT_TRUE(b.holds<int>());
    EXPECT_EQ(a.get_ref<std::string>(), std::string("text"));
    EXPECT_EQ(b.get<int>(), 99);
}


TEST_CASE(modifiers, swap_heap_heap)
{
    any a(std::string("alpha"));
    any b(std::string("beta"));

    a.swap(b);

    EXPECT_EQ(a.get_ref<std::string>(), std::string("beta"));
    EXPECT_EQ(b.get_ref<std::string>(), std::string("alpha"));
}
#endif


TEST_CASE(modifiers, swap_self)
{
    any a(42);

    a.swap(a);  // must remain a valid 42

    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.get<int>(), 42);
}


// ===========================================================================
// SWAP - FREE FUNCTION
// ===========================================================================

TEST_CASE(modifiers, free_swap)
{
    any a(1);
    any b(2);

    re_std::swap(a, b);

    EXPECT_EQ(a.get<int>(), 2);
    EXPECT_EQ(b.get<int>(), 1);
}


// ===========================================================================
// EMPLACE (C++11+ only)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

TEST_CASE(modifiers, emplace_sbo_int)
{
    any a;
    a.emplace<int>(123);

    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), 123);
}


TEST_CASE(modifiers, emplace_sbo_overwrites)
{
    any a(std::string("old"));
    a.emplace<int>(42);

    EXPECT_TRUE(a.holds<int>());
    EXPECT_FALSE(a.holds<std::string>());
    EXPECT_EQ(a.get<int>(), 42);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, emplace_heap)
{
    any a;
    a.emplace<std::string>("emplaced");

    EXPECT_TRUE(a.holds<std::string>());
    EXPECT_EQ(a.get_ref<std::string>(), std::string("emplaced"));
}


TEST_CASE(modifiers, emplace_heap_with_args)
{
    any a;
    a.emplace<std::string>(5, 'x');  // string(count, char)

    EXPECT_EQ(a.get_ref<std::string>(), std::string("xxxxx"));
}


TEST_CASE(modifiers, emplace_does_not_leak_previous)
{
    tests::Counted::reset_counts();

    any a(tests::Counted(1));
    int alive_before = tests::Counted::alive();
    EXPECT_TRUE(alive_before > 0);

    a.emplace<tests::Counted>(2);

    // previous Counted destroyed; new one alive
    EXPECT_EQ(tests::Counted::alive(), 1);
    EXPECT_EQ(a.get_ref<tests::Counted>().payload(), 2);
}


TEST_CASE(modifiers, emplace_initializer_list)
{
    any a;
    a.emplace<std::string>({'a', 'b', 'c'});

    EXPECT_EQ(a.get_ref<std::string>(), std::string("abc"));
}
#endif  // D_ENV_CPP98_HAS_NEW

#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// ===========================================================================
// COPY ASSIGNMENT
// ===========================================================================

TEST_CASE(modifiers, copy_assign_sbo)
{
    any a(1);
    any b;

    b = a;

    EXPECT_TRUE(b.holds<int>());
    EXPECT_EQ(b.get<int>(), 1);
    // source unaffected
    EXPECT_EQ(a.get<int>(), 1);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, copy_assign_heap_deep_copy)
{
    any a(std::string("source"));
    any b;

    b = a;

    EXPECT_TRUE(b.holds<std::string>());
    EXPECT_EQ(b.get_ref<std::string>(), std::string("source"));

    // independent storage
    b.get_ref<std::string>() = "modified";
    EXPECT_EQ(a.get_ref<std::string>(), std::string("source"));
}
#endif


TEST_CASE(modifiers, copy_assign_overwrites_different_type)
{
    any a(1);     // int
    any b(2.0);   // double

    a = b;

    EXPECT_FALSE(a.holds<int>());
    EXPECT_TRUE(a.holds<double>());
    EXPECT_EQ(a.get<double>(), 2.0);
}


TEST_CASE(modifiers, self_copy_assign_is_safe)
{
    any a(99);

    a = a;

    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.get<int>(), 99);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, copy_assign_heap_no_leak)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(1));
        any b(tests::Counted(2));
        b = a;
        // b previously held its own; that one should be destroyed
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif


// ===========================================================================
// MOVE ASSIGNMENT (C++11+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

TEST_CASE(modifiers, move_assign_sbo)
{
    any a(7);
    any b;

    b = static_cast<any&&>(a);

    EXPECT_TRUE(b.holds<int>());
    EXPECT_EQ(b.get<int>(), 7);
    EXPECT_FALSE(a.has_value());
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, move_assign_heap)
{
    any a(std::string("moved"));
    any b;

    b = static_cast<any&&>(a);

    EXPECT_TRUE(b.holds<std::string>());
    EXPECT_EQ(b.get_ref<std::string>(), std::string("moved"));
    EXPECT_FALSE(a.has_value());
}


TEST_CASE(modifiers, move_assign_overwrite_heap)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(1));
        any b(tests::Counted(2));

        b = static_cast<any&&>(a);
        // a is empty, b owns the originally-a Counted, the
        // originally-b Counted is destroyed
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif


TEST_CASE(modifiers, self_move_assign_is_safe)
{
    any a(42);

    any& ref = a;
    a = static_cast<any&&>(ref);

    EXPECT_TRUE(a.has_value());
}

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// ===========================================================================
// VALUE ASSIGNMENT (implicit via converting constructor + assignment)
// ===========================================================================

TEST_CASE(modifiers, assign_int_replaces_empty)
{
    any a;
    a = 5;

    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), 5);
}


TEST_CASE(modifiers, assign_replaces_type)
{
    any a(1);
    a = 2.5;

    EXPECT_TRUE(a.holds<double>());
    EXPECT_FALSE(a.holds<int>());
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(modifiers, assign_replaces_heap_with_sbo)
{
    any a(std::string("text"));
    a = 99;

    EXPECT_TRUE(a.holds<int>());
    EXPECT_FALSE(a.holds<std::string>());
}


TEST_CASE(modifiers, assign_replaces_sbo_with_heap)
{
    any a(42);
    a = std::string("text");

    EXPECT_TRUE(a.holds<std::string>());
    EXPECT_EQ(a.get_ref<std::string>(), std::string("text"));
}
#endif
