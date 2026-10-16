/*******************************************************************************
* djinterp [test]                                             test_retrieval.cpp
*
* re_std::any retrieval tests:
*   get<T>() (by-value retrieval, all SBO categories + heap)
*   get_ref<T>() (heap-only reference retrieval)
*
*
* path:      /tests/djinterp/re_std/any/test_retrieval.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "restd_any_tests.hpp"


using re_std::any;
namespace tests = restd_any_tests;


// ===========================================================================
// GET<T> - SBO PATH
// ===========================================================================

TEST_CASE(retrieval, get_bool_roundtrip)
{
    any a(true);
    EXPECT_EQ(a.get<bool>(), true);

    any b(false);
    EXPECT_EQ(b.get<bool>(), false);
}


TEST_CASE(retrieval, get_signed_full_range)
{
    EXPECT_EQ(any(-128).get<int>(), -128);
    EXPECT_EQ(any(127).get<int>(), 127);
    EXPECT_EQ(any(0).get<int>(),    0);
}


TEST_CASE(retrieval, get_unsigned_full_range)
{
    EXPECT_EQ(any(0u).get<unsigned int>(),   0u);
    EXPECT_EQ(any(1u).get<unsigned int>(),   1u);
    EXPECT_EQ(any(4294967295U).get<unsigned int>(), 4294967295U);
}


TEST_CASE(retrieval, get_floating_double)
{
    any a(3.14159);
    double v = a.get<double>();
    EXPECT_TRUE( (v > 3.14) && (v < 3.15) );
}


TEST_CASE(retrieval, get_floating_float)
{
    any a(2.5f);
    float v = a.get<float>();
    EXPECT_EQ(v, 2.5f);
}


TEST_CASE(retrieval, get_pointer_identity)
{
    int  x = 42;
    int* p = &x;
    any  a(p);

    int* recovered = a.get<int*>();
    EXPECT_EQ(recovered, p);
    EXPECT_EQ(*recovered, 42);
}


TEST_CASE(retrieval, get_const_pointer_identity)
{
    int        x  = 42;
    const int* cp = &x;
    any        a(cp);

    const int* recovered = a.get<const int*>();
    EXPECT_EQ(recovered, cp);
    EXPECT_EQ(*recovered, 42);
}


// ===========================================================================
// GET<T> - HEAP PATH
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(retrieval, get_heap_string)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    any a(std::string("hello"));
    const std::string& s = a.get<std::string>();
    EXPECT_EQ(s, std::string("hello"));
#else
    any a(std::string("hello"));
    std::string s = a.get<std::string>();
    EXPECT_EQ(s, std::string("hello"));
#endif
}


TEST_CASE(retrieval, get_heap_aggregate)
{
    tests::Aggregate orig(11, 22.5);
    any              a(orig);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const tests::Aggregate& got = a.get<tests::Aggregate>();
#else
    tests::Aggregate got = a.get<tests::Aggregate>();
#endif

    EXPECT_TRUE(got == orig);
}


TEST_CASE(retrieval, get_heap_largetype)
{
    tests::LargeType lt(123);
    any              a(lt);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const tests::LargeType& got = a.get<tests::LargeType>();
#else
    tests::LargeType got = a.get<tests::LargeType>();
#endif

    EXPECT_TRUE(got == lt);
}
#endif


// ===========================================================================
// GET_REF<T> - HEAP-ONLY
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(retrieval, get_ref_mutates_underlying)
{
    any a(std::string("before"));

    std::string& ref = a.get_ref<std::string>();
    EXPECT_EQ(ref, std::string("before"));

    ref = "after";
    EXPECT_EQ(a.get_ref<std::string>(), std::string("after"));
}


TEST_CASE(retrieval, get_ref_const_overload)
{
    any         a(std::string("immutable"));
    const any&  ca = a;

    const std::string& ref = ca.get_ref<std::string>();
    EXPECT_EQ(ref, std::string("immutable"));
}


TEST_CASE(retrieval, get_ref_independent_per_any)
{
    any a(std::string("apple"));
    any b(std::string("banana"));

    EXPECT_NE(&a.get_ref<std::string>(), &b.get_ref<std::string>());
}
#endif


// ===========================================================================
// TYPE COERCION (NEGATIVE - get<T> WITH WRONG T IS UNDEFINED)
// ===========================================================================
// We don't test wrong-type get<T>() directly because it's undefined.
// Use any_cast for checked retrieval; tested in test_any_cast.cpp.


// ===========================================================================
// CHAINED RETRIEVAL
// ===========================================================================

TEST_CASE(retrieval, get_multiple_times)
{
    any a(99);
    EXPECT_EQ(a.get<int>(), 99);
    EXPECT_EQ(a.get<int>(), 99);
    EXPECT_EQ(a.get<int>(), 99);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(retrieval, get_ref_then_mutate_then_get)
{
    any a(std::string("v1"));

    a.get_ref<std::string>() += "-v2";
    a.get_ref<std::string>() += "-v3";

    EXPECT_EQ(a.get_ref<std::string>(), std::string("v1-v2-v3"));
}
#endif
