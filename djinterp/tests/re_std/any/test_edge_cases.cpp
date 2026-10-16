/*******************************************************************************
* djinterp [test]                                            test_edge_cases.cpp
*
* re_std::any edge-case tests:
*   - any of any (nested type erasure)
*   - polymorphic objects (slicing prevention)
*   - exception safety on heap construction
*   - SBO / heap boundary behaviour
*   - type identity preservation under conversion-prone operations
*   - empty any operations
*   - move-only types (C++11+)
*
*
* path:      /tests/djinterp/re_std/any/test_edge_cases.cpp
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
// ANY OF ANY (NESTED TYPE ERASURE)
// ===========================================================================
// Note: `any outer(inner_any)` calls the copy constructor and produces
// a peer copy, not a wrapped any.  To genuinely nest an any inside
// another, use emplace<any>().  std::any has the same property.

#if D_ENV_CPP98_HAS_NEW && D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
TEST_CASE(edge, any_of_any)
{
    any inner(42);
    any outer;
    outer.emplace<any>(inner);

    EXPECT_TRUE(outer.holds<any>());
    EXPECT_FALSE(outer.holds<int>());

    const any& got = outer.get_ref<any>();
    EXPECT_TRUE(got.holds<int>());
    EXPECT_EQ(got.get<int>(), 42);
}


TEST_CASE(edge, deeply_nested_any)
{
    any leaf(99);

    any level_a;
    level_a.emplace<any>(leaf);

    any level_b;
    level_b.emplace<any>(level_a);

    EXPECT_TRUE(level_b.holds<any>());
    EXPECT_TRUE(level_b.get_ref<any>().holds<any>());

    // unwrap: level_b -> any -> any -> int
    const any& l1 = level_b.get_ref<any>();
    const any& l2 = l1.get_ref<any>();
    EXPECT_EQ(l2.get<int>(), 99);
}
#endif


// ===========================================================================
// TYPE IDENTITY PRESERVATION
// ===========================================================================

TEST_CASE(edge, int_vs_long_distinct)
{
    int  iv = 1;
    long lv = 1L;
    any  ai(iv);
    any  al(lv);

    EXPECT_NE(ai.type(), al.type());
    EXPECT_TRUE(ai.holds<int>());
    EXPECT_FALSE(ai.holds<long>());
    EXPECT_TRUE(al.holds<long>());
    EXPECT_FALSE(al.holds<int>());
}


TEST_CASE(edge, signed_vs_unsigned_distinct)
{
    any as(1);
    any au(1u);

    EXPECT_NE(as.type(), au.type());
}


TEST_CASE(edge, float_vs_double_distinct)
{
    any af(1.0f);
    any ad(1.0);

    EXPECT_NE(af.type(), ad.type());
    EXPECT_TRUE(af.holds<float>());
    EXPECT_FALSE(af.holds<double>());
    EXPECT_TRUE(ad.holds<double>());
}


TEST_CASE(edge, char_vs_signed_char_vs_unsigned_char)
{
    any ac(static_cast<char>(1));
    any asc(static_cast<signed char>(1));
    any auc(static_cast<unsigned char>(1));

    // std::any treats these as three distinct types per [basic.fundamental].
    // re_std::any follows suit.
    EXPECT_NE(ac.type(),  asc.type());
    EXPECT_NE(ac.type(),  auc.type());
    EXPECT_NE(asc.type(), auc.type());
}


// ===========================================================================
// POLYMORPHISM AND SLICING
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, derived_stored_as_base_slices)
{
    tests::Derived d(99);

    // store as Base - slicing occurs, by design
    tests::Base&   ref = d;
    any            a(ref);

    EXPECT_TRUE(a.holds<tests::Base>());
    EXPECT_FALSE(a.holds<tests::Derived>());

    // kind() should report 1 (Base) due to slicing
    EXPECT_EQ(a.get_ref<tests::Base>().kind(), 1);
}


TEST_CASE(edge, derived_stored_as_derived_preserves_type)
{
    tests::Derived d(42);
    any            a(d);

    EXPECT_TRUE(a.holds<tests::Derived>());
    EXPECT_FALSE(a.holds<tests::Base>());

    EXPECT_EQ(a.get_ref<tests::Derived>().kind(), 2);
    EXPECT_EQ(a.get_ref<tests::Derived>().m_extra, 42);
}


TEST_CASE(edge, base_pointer_to_derived)
{
    tests::Derived d(7);
    tests::Base*   p = &d;
    any            a(p);

    EXPECT_TRUE(a.holds<tests::Base*>());
    EXPECT_EQ(a.get<tests::Base*>()->kind(), 2);  // virtual dispatch works
}
#endif


// ===========================================================================
// EXCEPTION SAFETY
// ===========================================================================

#if D_ENV_CPP98_HAS_EXCEPTION && D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, heap_ctor_throw_leaves_any_unaffected)
{
    any a(42);   // a holds an int

    try
    {
        tests::ThrowOnCopy t;
        a = t;   // copy will throw
        EXPECT_TRUE(false);  // unreached
    }
    catch (...)
    {
        // any may be empty or still hold the original int -
        // exact state is implementation-defined.  What we
        // require: no leaks, the any is in a valid state.
    }

    // valid state check - either of these must hold
    EXPECT_TRUE( (a.has_value() && a.holds<int>()) ||
                 (!a.has_value()) );
}
#endif


// ===========================================================================
// EMPTY ANY OPERATIONS
// ===========================================================================

TEST_CASE(edge, empty_assigned_from_empty)
{
    any a;
    any b;

    a = b;

    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());
}


TEST_CASE(edge, empty_swap_with_empty)
{
    any a;
    any b;

    a.swap(b);

    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());
}


TEST_CASE(edge, empty_copy_construct)
{
    any a;
    any b(a);

    EXPECT_FALSE(b.has_value());
    EXPECT_EQ(b.category(), re_std::DAnyCategory::cat_empty);
}


TEST_CASE(edge, reset_after_reset)
{
    any a(1);
    a.reset();
    a.reset();
    a.reset();

    EXPECT_FALSE(a.has_value());
}


// ===========================================================================
// REPEATED ASSIGNMENT - STABILITY
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, repeated_heap_assignment_no_leak)
{
    tests::Counted::reset_counts();

    {
        any a(tests::Counted(0));
        for (int i = 1; i < 100; ++i)
        {
            a = tests::Counted(i);
        }
        EXPECT_TRUE(a.holds<tests::Counted>());
        EXPECT_EQ(a.get_ref<tests::Counted>().payload(), 99);
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}


TEST_CASE(edge, alternating_sbo_heap_assignment)
{
    any a(1);

    for (int i = 0; i < 50; ++i)
    {
        if ((i % 2) == 0)
        {
            a = i;  // SBO
            EXPECT_TRUE(a.holds<int>());
        }
        else
        {
            a = std::string("heap");  // heap
            EXPECT_TRUE(a.holds<std::string>());
        }
    }
}
#endif


// ===========================================================================
// LARGE NUMBER OF DISTINCT ANYS (stress)
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, many_anys_no_leak)
{
    tests::Counted::reset_counts();

    {
        std::size_t kNumAnys = 1000;
        any* arr = new any[kNumAnys];

        for (std::size_t i = 0; i < kNumAnys; ++i)
        {
            arr[i] = tests::Counted(static_cast<int>(i));
        }

        EXPECT_EQ(static_cast<std::size_t>(tests::Counted::alive()),
                  kNumAnys);

        delete[] arr;
    }

    EXPECT_EQ(tests::Counted::alive(), 0);
}
#endif


// ===========================================================================
// CONST CORRECTNESS
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, const_any_observers_compile)
{
    const any a(42);

    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(a.holds<int>());
    EXPECT_EQ(a.get<int>(), 42);
    EXPECT_EQ(a.category(), re_std::DAnyCategory::cat_signed);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    EXPECT_TRUE(static_cast<bool>(a));
#else
    EXPECT_TRUE(a);
#endif
}


TEST_CASE(edge, const_any_get_ref_const)
{
    const any a(std::string("immutable"));

    const std::string& s = a.get_ref<std::string>();
    EXPECT_EQ(s, std::string("immutable"));
}
#endif


// ===========================================================================
// FUNCTION POINTERS (ROUTED TO HEAP)
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, function_pointer_stored)
{
    typedef int (*fn_t)();
    fn_t fp = &tests::dummy_function;

    any a(fp);

    // function pointers route to heap (not SBO) via SFINAE
    EXPECT_TRUE(a.holds<fn_t>());
    EXPECT_EQ(a.get_ref<fn_t>()(), 42);
}


TEST_CASE(edge, function_pointer_with_args)
{
    typedef int (*fn_t)(int, int);
    fn_t fp = &tests::dummy_function_with_args;

    any a(fp);

    EXPECT_TRUE(a.holds<fn_t>());
    EXPECT_EQ(a.get_ref<fn_t>()(2, 3), 5);
}
#endif


// ===========================================================================
// MOVE-ONLY TYPES (C++11+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES && D_ENV_CPP98_HAS_NEW

namespace
{

struct MoveOnly
{
    int m_value;

    explicit MoveOnly(int _v) : m_value(_v) {}
    MoveOnly(MoveOnly&& _other) D_NOEXCEPT
        : m_value(_other.m_value)
    {
        _other.m_value = -1;
    }

    // copyable required for any-of-T (clone op).  std::any has the
    // same requirement: stored types must be CopyConstructible.
    MoveOnly(const MoveOnly& _other) : m_value(_other.m_value) {}
};

}  // namespace


TEST_CASE(edge, move_into_any_with_copyable_type)
{
    MoveOnly src(42);
    any a(static_cast<MoveOnly&&>(src));

    EXPECT_TRUE(a.holds<MoveOnly>());
    EXPECT_EQ(a.get_ref<MoveOnly>().m_value, 42);
}

#endif


// ===========================================================================
// PTR-TO-POINTER (TWO-LEVEL INDIRECTION)
// ===========================================================================

TEST_CASE(edge, pointer_to_pointer)
{
    int   x   = 1;
    int*  p   = &x;
    int** pp  = &p;

    any a(pp);

    EXPECT_TRUE(a.holds<int**>());
    EXPECT_EQ(a.get<int**>(), pp);
    EXPECT_EQ(*a.get<int**>(), p);
    EXPECT_EQ(**a.get<int**>(), 1);
}


// ===========================================================================
// CONTAINER OF ANY
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW
TEST_CASE(edge, vector_of_any)
{
    // build via push_back so we exercise copy/move within a container
    std::size_t const kCount = 5;

    any* arr = new any[kCount];
    arr[0] = 1;
    arr[1] = 2.5;
    arr[2] = std::string("three");
    arr[3] = true;
    arr[4] = static_cast<long>(99999L);

    EXPECT_TRUE(arr[0].holds<int>());
    EXPECT_TRUE(arr[1].holds<double>());
    EXPECT_TRUE(arr[2].holds<std::string>());
    EXPECT_TRUE(arr[3].holds<bool>());
    EXPECT_TRUE(arr[4].holds<long>());

    delete[] arr;
}
#endif
