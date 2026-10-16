/*******************************************************************************
* djinterp [test]                                             test_observers.cpp
*
* re_std::any observer tests:
*   has_value(), category(), type(), holds<T>(), is_sbo(),
* operator bool / safe-bool.
*
*
* path:      /tests/djinterp/re_std/any/test_observers.cpp
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
// HAS_VALUE
// ===========================================================================

TEST_CASE(observers, has_value_empty)
{
    any a;
    EXPECT_FALSE(a.has_value());
}


TEST_CASE(observers, has_value_after_construct)
{
    any a(42);
    EXPECT_TRUE(a.has_value());
}


TEST_CASE(observers, has_value_after_reset)
{
    any a(42);
    a.reset();
    EXPECT_FALSE(a.has_value());
}


// ===========================================================================
// OPERATOR BOOL / SAFE-BOOL
// ===========================================================================

TEST_CASE(observers, bool_true_when_populated)
{
    any a(1);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    EXPECT_TRUE(static_cast<bool>(a));
#else
    EXPECT_TRUE(a);
#endif
}


TEST_CASE(observers, bool_false_when_empty)
{
    any a;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    EXPECT_FALSE(static_cast<bool>(a));
#else
    EXPECT_FALSE(a);
#endif
}


// safe-bool should not permit implicit conversion to int.  We can't
// negative-test this at runtime, but a smoke test of usage in an
// if-statement context still has value.
TEST_CASE(observers, bool_works_in_conditional)
{
    any a(7);

    int hit = 0;
    if (a)
    {
        hit = 1;
    }

    EXPECT_EQ(hit, 1);
}


// ===========================================================================
// CATEGORY
// ===========================================================================

TEST_CASE(observers, category_empty)
{
    any a;
    EXPECT_EQ(a.category(), DAnyCategory::cat_empty);
}


TEST_CASE(observers, category_bool)
{
    any a(true);
    EXPECT_EQ(a.category(), DAnyCategory::cat_bool);
}


TEST_CASE(observers, category_signed)
{
    any a(-1);
    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
}


TEST_CASE(observers, category_unsigned)
{
    any a(7u);
    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
}


TEST_CASE(observers, category_floating)
{
    any a(1.5);
    EXPECT_EQ(a.category(), DAnyCategory::cat_floating);
}


TEST_CASE(observers, category_pointer)
{
    int x;
    any a(&x);
    EXPECT_EQ(a.category(), DAnyCategory::cat_pointer);
}


TEST_CASE(observers, category_cpointer)
{
    const int  x = 0;
    const int* p = &x;
    any        a(p);
    EXPECT_EQ(a.category(), DAnyCategory::cat_cpointer);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(observers, category_heap)
{
    any a(std::string("heap"));
    EXPECT_EQ(a.category(), DAnyCategory::cat_heap);
}
#endif


// ===========================================================================
// TYPE
// ===========================================================================

TEST_CASE(observers, type_empty_is_null)
{
    any a;
    EXPECT_EQ(a.type(), static_cast<re_std::any_type_id>(0));
}


TEST_CASE(observers, type_distinct_per_type)
{
    any ai(1);
    any al(1L);
    any af(1.0f);
    any ad(1.0);

    EXPECT_NE(ai.type(), al.type());
    EXPECT_NE(ai.type(), af.type());
    EXPECT_NE(ai.type(), ad.type());
    EXPECT_NE(af.type(), ad.type());
}


TEST_CASE(observers, type_stable_across_copy)
{
    any a(42);
    any b(a);

    EXPECT_EQ(a.type(), b.type());
}


// ===========================================================================
// HOLDS<T>
// ===========================================================================

TEST_CASE(observers, holds_correct_for_each_integral)
{
    any ai(int(1));
    any al(long(1));
#if D_ENV_HAS_LONG_LONG
    any all(static_cast<long long>(1));
#endif
    any au(static_cast<unsigned int>(1));

    EXPECT_TRUE(ai.holds<int>());
    EXPECT_FALSE(ai.holds<long>());
    EXPECT_FALSE(ai.holds<unsigned int>());

    EXPECT_TRUE(al.holds<long>());
    EXPECT_FALSE(al.holds<int>());

#if D_ENV_HAS_LONG_LONG
    EXPECT_TRUE(all.holds<long long>());
    EXPECT_FALSE(all.holds<long>());
#endif

    EXPECT_TRUE(au.holds<unsigned int>());
    EXPECT_FALSE(au.holds<int>());
}


TEST_CASE(observers, holds_correct_for_each_float)
{
    any af(1.0f);
    any ad(1.0);

    EXPECT_TRUE(af.holds<float>());
    EXPECT_FALSE(af.holds<double>());
    EXPECT_TRUE(ad.holds<double>());
    EXPECT_FALSE(ad.holds<float>());
}


TEST_CASE(observers, holds_pointer_vs_const_pointer)
{
    int        x = 0;
    int*       p = &x;
    const int* q = &x;

    any a(p);
    any b(q);

    EXPECT_TRUE(a.holds<int*>());
    EXPECT_FALSE(a.holds<const int*>());
    EXPECT_TRUE(b.holds<const int*>());
    EXPECT_FALSE(b.holds<int*>());
}


TEST_CASE(observers, holds_negative_on_empty)
{
    any a;

    EXPECT_FALSE(a.holds<int>());
    EXPECT_FALSE(a.holds<double>());
    EXPECT_FALSE(a.holds<std::string>());
}


// ===========================================================================
// IS_SBO
// ===========================================================================

TEST_CASE(observers, is_sbo_for_primitives)
{
    EXPECT_TRUE(any(true).is_sbo());
    EXPECT_TRUE(any(1).is_sbo());
    EXPECT_TRUE(any(1u).is_sbo());
    EXPECT_TRUE(any(1.0).is_sbo());

    int x;
    EXPECT_TRUE(any(&x).is_sbo());
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(observers, is_sbo_false_for_heap)
{
    any a(std::string("heap"));
    EXPECT_FALSE(a.is_sbo());
}
#endif


TEST_CASE(observers, is_sbo_false_for_empty)
{
    any a;
    EXPECT_FALSE(a.is_sbo());
}


// ===========================================================================
// OBSERVERS NOEXCEPT (compile-time check)
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
TEST_CASE(observers, observers_are_noexcept)
{
    any a(1);

    static_assert(noexcept(a.has_value()), "has_value must be noexcept");
    static_assert(noexcept(a.category()),  "category must be noexcept");
    static_assert(noexcept(a.type()),      "type must be noexcept");
    static_assert(noexcept(a.holds<int>()), "holds must be noexcept");
    static_assert(noexcept(a.is_sbo()),    "is_sbo must be noexcept");

    EXPECT_TRUE(true);  // reaches here only if static_asserts pass
}
#endif
