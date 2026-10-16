/*******************************************************************************
* djinterp [test]                                          test_construction.cpp
*
* re_std::any construction tests:
*   Exercises every constructor path on every C++ standard, plus
* the resulting category / type-id observers immediately after
* construction.
*
* Covered:
*   - default
*   - bool, signed/unsigned integrals, char, wchar_t, floating
*   - non-const / const pointer, null pointer, void pointer
*   - enum (SBO when intrinsics available; otherwise via underlying)
*   - copy
*   - move (C++11+)
*   - heap (class type)
*   - long long / unsigned long long (gated)
*
*
* path:      /tests/djinterp/re_std/any/test_construction.cpp
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
// DEFAULT CONSTRUCTION
// ===========================================================================

TEST_CASE(construction, default_constructs_empty)
{
    any a;

    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.category(), DAnyCategory::cat_empty);
    EXPECT_EQ(a.type(), static_cast<re_std::any_type_id>(0));
    EXPECT_FALSE(a.is_sbo());
}


TEST_CASE(construction, default_operator_bool_false)
{
    any a;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    EXPECT_FALSE(static_cast<bool>(a));
#else
    EXPECT_FALSE(a);
#endif
}


// ===========================================================================
// BOOL
// ===========================================================================

TEST_CASE(construction, bool_true)
{
    any a(true);

    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(a.is_sbo());
    EXPECT_EQ(a.category(), DAnyCategory::cat_bool);
    EXPECT_TRUE(a.holds<bool>());
    EXPECT_EQ(a.get<bool>(), true);
}


TEST_CASE(construction, bool_false)
{
    any a(false);

    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.category(), DAnyCategory::cat_bool);
    EXPECT_EQ(a.get<bool>(), false);
}


// ===========================================================================
// SIGNED INTEGRALS
// ===========================================================================

TEST_CASE(construction, signed_char)
{
    signed char v = -7;
    any         a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
    EXPECT_TRUE(a.holds<signed char>());
    EXPECT_EQ(static_cast<int>(a.get<signed char>()), -7);
}


TEST_CASE(construction, short_int)
{
    short v = -32000;
    any   a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
    EXPECT_TRUE(a.holds<short>());
    EXPECT_EQ(a.get<short>(), static_cast<short>(-32000));
}


TEST_CASE(construction, int_negative)
{
    any a(-12345);

    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), -12345);
}


TEST_CASE(construction, long_int)
{
    long v = -2147483647L;
    any  a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
    EXPECT_TRUE(a.holds<long>());
    EXPECT_EQ(a.get<long>(), -2147483647L);
}


#if D_ENV_HAS_LONG_LONG
TEST_CASE(construction, long_long_int)
{
    long long v = -9000000000LL;
    any       a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_signed);
    EXPECT_TRUE(a.holds<long long>());
    EXPECT_EQ(a.get<long long>(), -9000000000LL);
}
#endif


// ===========================================================================
// UNSIGNED INTEGRALS
// ===========================================================================

TEST_CASE(construction, unsigned_char)
{
    unsigned char v = 200;
    any           a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
    EXPECT_TRUE(a.holds<unsigned char>());
    EXPECT_EQ(static_cast<int>(a.get<unsigned char>()), 200);
}


TEST_CASE(construction, unsigned_short)
{
    unsigned short v = 60000U;
    any            a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
    EXPECT_TRUE(a.holds<unsigned short>());
    EXPECT_EQ(a.get<unsigned short>(), static_cast<unsigned short>(60000U));
}


TEST_CASE(construction, unsigned_int)
{
    unsigned int v = 4000000000U;
    any          a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
    EXPECT_TRUE(a.holds<unsigned int>());
    EXPECT_EQ(a.get<unsigned int>(), 4000000000U);
}


TEST_CASE(construction, unsigned_long)
{
    unsigned long v = 1234567890UL;
    any           a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
    EXPECT_TRUE(a.holds<unsigned long>());
    EXPECT_EQ(a.get<unsigned long>(), 1234567890UL);
}


#if D_ENV_HAS_LONG_LONG
TEST_CASE(construction, unsigned_long_long)
{
    unsigned long long v = 18000000000ULL;
    any                a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_unsigned);
    EXPECT_TRUE(a.holds<unsigned long long>());
    EXPECT_EQ(a.get<unsigned long long>(), 18000000000ULL);
}
#endif


// ===========================================================================
// CHAR / WCHAR_T (PLATFORM-DEPENDENT SIGNEDNESS)
// ===========================================================================

TEST_CASE(construction, plain_char)
{
    char c = 'A';
    any  a(c);

    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(a.is_sbo());
    EXPECT_TRUE(a.holds<char>());
    EXPECT_EQ(a.get<char>(), 'A');

    // category must be signed or unsigned (not bool)
    EXPECT_TRUE( (a.category() == DAnyCategory::cat_signed) ||
                 (a.category() == DAnyCategory::cat_unsigned) );
}


TEST_CASE(construction, wide_char)
{
    wchar_t w = L'Z';
    any     a(w);

    EXPECT_TRUE(a.holds<wchar_t>());
    EXPECT_EQ(a.get<wchar_t>(), L'Z');
}


// ===========================================================================
// FLOATING POINT
// ===========================================================================

TEST_CASE(construction, float_value)
{
    any a(3.14f);

    EXPECT_EQ(a.category(), DAnyCategory::cat_floating);
    EXPECT_TRUE(a.holds<float>());
    EXPECT_TRUE(a.get<float>() > 3.13f);
    EXPECT_TRUE(a.get<float>() < 3.15f);
}


TEST_CASE(construction, double_value)
{
    any a(2.71828);

    EXPECT_EQ(a.category(), DAnyCategory::cat_floating);
    EXPECT_TRUE(a.holds<double>());
    EXPECT_TRUE(a.get<double>() > 2.718);
    EXPECT_TRUE(a.get<double>() < 2.719);
}


TEST_CASE(construction, long_double_value)
{
    long double v = 1.41421356L;
    any         a(v);

    EXPECT_EQ(a.category(), DAnyCategory::cat_floating);
    EXPECT_TRUE(a.holds<long double>());
}


// ===========================================================================
// POINTERS
// ===========================================================================

TEST_CASE(construction, mutable_pointer)
{
    int  x = 42;
    int* p = &x;
    any  a(p);

    EXPECT_EQ(a.category(), DAnyCategory::cat_pointer);
    EXPECT_TRUE(a.holds<int*>());
    EXPECT_EQ(a.get<int*>(), &x);
}


TEST_CASE(construction, const_pointer)
{
    int        x = 42;
    const int* p = &x;
    any        a(p);

    EXPECT_EQ(a.category(), DAnyCategory::cat_cpointer);
    EXPECT_TRUE(a.holds<const int*>());
    EXPECT_EQ(a.get<const int*>(), &x);
}


TEST_CASE(construction, null_pointer_int)
{
    int* p = D_NULLPTR;
    any  a(p);

    EXPECT_EQ(a.category(), DAnyCategory::cat_pointer);
    EXPECT_TRUE(a.holds<int*>());
    EXPECT_EQ(a.get<int*>(), static_cast<int*>(D_NULLPTR));
}


TEST_CASE(construction, void_pointer)
{
    int   x = 99;
    void* p = &x;
    any   a(p);

    EXPECT_EQ(a.category(), DAnyCategory::cat_pointer);
    EXPECT_TRUE(a.holds<void*>());
    EXPECT_EQ(a.get<void*>(), &x);
}


TEST_CASE(construction, const_void_pointer)
{
    int         x = 99;
    const void* p = &x;
    any         a(p);

    EXPECT_EQ(a.category(), DAnyCategory::cat_cpointer);
    EXPECT_TRUE(a.holds<const void*>());
    EXPECT_EQ(a.get<const void*>(), &x);
}


TEST_CASE(construction, pointer_and_const_pointer_distinct)
{
    int        x = 1;
    int*       p1 = &x;
    const int* p2 = &x;

    any a1(p1);
    any a2(p2);

    EXPECT_NE(a1.type(), a2.type());
    EXPECT_TRUE(a1.holds<int*>());
    EXPECT_FALSE(a1.holds<const int*>());
    EXPECT_TRUE(a2.holds<const int*>());
    EXPECT_FALSE(a2.holds<int*>());
}


// ===========================================================================
// ENUM
// ===========================================================================

#if D_RE_STD_HAS_IS_ENUM && D_RE_STD_HAS_UNDERLYING_TYPE
TEST_CASE(construction, unscoped_enum)
{
    any a(tests::se_two);

    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(a.is_sbo());
    EXPECT_TRUE(a.holds<tests::SmallEnum>());
    EXPECT_EQ(a.get<tests::SmallEnum>(), tests::se_two);
}

#if D_ENV_CPP_FEATURE_LANG_STRONG_ENUMS
TEST_CASE(construction, scoped_enum)
{
    any a(tests::ScopedEnum::sc_one);

    EXPECT_TRUE(a.holds<tests::ScopedEnum>());
    EXPECT_TRUE(a.get<tests::ScopedEnum>() == tests::ScopedEnum::sc_one);
}
#endif
#endif


// ===========================================================================
// HEAP
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(construction, heap_std_string)
{
    std::string s("hello world");
    any         a(s);

    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(a.is_sbo());
    EXPECT_EQ(a.category(), DAnyCategory::cat_heap);
    EXPECT_TRUE(a.holds<std::string>());
}


TEST_CASE(construction, heap_aggregate)
{
    tests::Aggregate orig(7, 8.5);
    any              a(orig);

    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(a.is_sbo());
    EXPECT_TRUE(a.holds<tests::Aggregate>());
}


TEST_CASE(construction, heap_largetype)
{
    tests::LargeType lt(99);
    any              a(lt);

    EXPECT_TRUE(a.holds<tests::LargeType>());
    EXPECT_FALSE(a.is_sbo());
}
#endif


// ===========================================================================
// COPY
// ===========================================================================

TEST_CASE(construction, copy_sbo)
{
    any a(42);
    any b(a);

    EXPECT_EQ(a.category(), b.category());
    EXPECT_EQ(a.type(),     b.type());
    EXPECT_EQ(a.get<int>(), b.get<int>());
    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(b.has_value());
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(construction, copy_heap)
{
    any a(std::string("copy me"));
    any b(a);

    EXPECT_TRUE(a.holds<std::string>());
    EXPECT_TRUE(b.holds<std::string>());
    EXPECT_EQ(a.get_ref<std::string>(), b.get_ref<std::string>());

    // mutating b's heap copy must not touch a
    b.get_ref<std::string>() = "altered";
    EXPECT_NE(a.get_ref<std::string>(), b.get_ref<std::string>());
}
#endif


TEST_CASE(construction, copy_empty)
{
    any a;
    any b(a);

    EXPECT_FALSE(b.has_value());
    EXPECT_EQ(b.category(), DAnyCategory::cat_empty);
}


// ===========================================================================
// MOVE (C++11+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
TEST_CASE(construction, move_sbo)
{
    any a(123);
    any b(static_cast<any&&>(a));

    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(b.get<int>(), 123);
    EXPECT_FALSE(a.has_value());
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(construction, move_heap)
{
    any a(std::string("move me"));
    any b(static_cast<any&&>(a));

    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_TRUE(b.holds<std::string>());
    EXPECT_EQ(b.get_ref<std::string>(), std::string("move me"));
}


TEST_CASE(construction, move_does_not_clone)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(5));
        int before = tests::Counted::alive();
        any b(static_cast<any&&>(a));
        int after  = tests::Counted::alive();

        // move must not allocate a new Counted
        EXPECT_EQ(before, after);
        (void)b;
    }

    // post-scope: all Counted destroyed
    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif


TEST_CASE(construction, move_empty)
{
    any a;
    any b(static_cast<any&&>(a));

    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());
}
#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// ===========================================================================
// LEAK CHECK
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(construction, no_leak_on_temporary)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(7));
        EXPECT_TRUE(a.holds<tests::Counted>());
        // a goes out of scope here
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}


TEST_CASE(construction, no_leak_chain_of_copies)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(1));
        any b(a);
        any c(b);
        any d(c);
        (void)d;
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif
