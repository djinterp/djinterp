/*******************************************************************************
* djinterp [test]                                              test_any_cast.cpp
*
* re_std::any_cast tests:
*   All overloads of any_cast:
*     any_cast<T>(any*)       -> T*       (heap-only, nullptr on mismatch)
*     any_cast<T>(const any*) -> const T*
*     any_cast<T>(const any&) -> T        (throws bad_any_cast on mismatch)
*     any_cast<T>(any&)       -> T&       (heap-only)
*     any_cast<T>(any&&)      -> T        (C++11+)
*
*
* path:      /tests/djinterp/re_std/any/test_any_cast.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "restd_any_tests.hpp"


using re_std::any;
using re_std::any_cast;
using re_std::bad_any_cast;
namespace tests = restd_any_tests;


// ===========================================================================
// POINTER OVERLOADS - HEAP ONLY
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW

TEST_CASE(any_cast, pointer_match_heap)
{
    any         a(std::string("ptr_match"));
    std::string* p = any_cast<std::string>(&a);

    EXPECT_TRUE(p != static_cast<std::string*>(D_NULLPTR));
    EXPECT_EQ(*p, std::string("ptr_match"));
}


TEST_CASE(any_cast, pointer_mismatch_returns_null)
{
    any  a(std::string("text"));
    int* p = any_cast<int>(&a);

    EXPECT_EQ(p, static_cast<int*>(D_NULLPTR));
}


TEST_CASE(any_cast, pointer_null_any_returns_null)
{
    any* nullp = D_NULLPTR;
    std::string* result = any_cast<std::string>(nullp);

    EXPECT_EQ(result, static_cast<std::string*>(D_NULLPTR));
}


TEST_CASE(any_cast, const_pointer_match_heap)
{
    any              a(std::string("const_ptr"));
    const any&       ca = a;
    const std::string* p = any_cast<std::string>(&ca);

    EXPECT_TRUE(p != static_cast<const std::string*>(D_NULLPTR));
    EXPECT_EQ(*p, std::string("const_ptr"));
}


TEST_CASE(any_cast, const_pointer_mismatch_returns_null)
{
    any        a(std::string("text"));
    const any& ca = a;
    const int* p  = any_cast<int>(&ca);

    EXPECT_EQ(p, static_cast<const int*>(D_NULLPTR));
}


TEST_CASE(any_cast, pointer_mutate_through)
{
    any          a(std::string("v1"));
    std::string* p = any_cast<std::string>(&a);

    *p = "v2";

    EXPECT_EQ(a.get_ref<std::string>(), std::string("v2"));
}

#endif  // D_ENV_CPP98_HAS_NEW


// ===========================================================================
// CONST LVALUE REF OVERLOAD - CHECKED COPY
// ===========================================================================

#if D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO

TEST_CASE(any_cast, const_ref_match_sbo)
{
    any        a(42);
    const any& ca = a;

    int v = any_cast<int>(ca);

    EXPECT_EQ(v, 42);
}


#if D_ENV_CPP98_HAS_NEW
TEST_CASE(any_cast, const_ref_match_heap)
{
    any         a(std::string("ref_match"));
    const any&  ca = a;

    std::string v = any_cast<std::string>(ca);

    EXPECT_EQ(v, std::string("ref_match"));
}
#endif


TEST_CASE(any_cast, const_ref_mismatch_throws)
{
    any        a(42);
    const any& ca = a;

    EXPECT_THROWS(any_cast<double>(ca), bad_any_cast);
}


TEST_CASE(any_cast, const_ref_empty_throws)
{
    any        a;
    const any& ca = a;

    EXPECT_THROWS(any_cast<int>(ca), bad_any_cast);
}


// ===========================================================================
// MUTABLE LVALUE REF OVERLOAD - HEAP-ONLY
// ===========================================================================

#if D_ENV_CPP98_HAS_NEW

TEST_CASE(any_cast, mutable_ref_match_heap)
{
    any a(std::string("mut"));

    std::string& r = any_cast<std::string&>(a);
    r = "altered";

    EXPECT_EQ(a.get_ref<std::string>(), std::string("altered"));
}


TEST_CASE(any_cast, mutable_ref_mismatch_throws)
{
    any a(std::string("x"));

    EXPECT_THROWS(any_cast<int&>(a), bad_any_cast);
}


// ===========================================================================
// RVALUE REF OVERLOAD (C++11+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
TEST_CASE(any_cast, rvalue_match_heap)
{
    any a(std::string("rvalue"));

    std::string moved = any_cast<std::string>(static_cast<any&&>(a));

    EXPECT_EQ(moved, std::string("rvalue"));
}


TEST_CASE(any_cast, rvalue_mismatch_throws)
{
    any a(std::string("x"));

    EXPECT_THROWS(any_cast<int>(static_cast<any&&>(a)), bad_any_cast);
}
#endif

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO


// ===========================================================================
// BAD_ANY_CAST WHAT()
// ===========================================================================

#if D_ENV_CPP98_HAS_EXCEPTION || D_ENV_CPP98_HAS_TYPEINFO
TEST_CASE(any_cast, bad_any_cast_what)
{
    bad_any_cast e;
    const char*  msg = e.what();

    EXPECT_TRUE(msg != static_cast<const char*>(D_NULLPTR));
    // we don't assert the exact text, just non-empty
    EXPECT_TRUE(msg[0] != '\0');
}


#if D_ENV_CPP98_HAS_TYPEINFO
TEST_CASE(any_cast, bad_any_cast_inherits_bad_cast)
{
    try
    {
        any a(1);
        any_cast<double>(static_cast<const any&>(a));
        EXPECT_TRUE(false);  // unreached
    }
    catch (const std::bad_cast& _e)
    {
        EXPECT_TRUE(true);
        (void)_e;
    }
}
#endif

#if D_ENV_CPP98_HAS_EXCEPTION
TEST_CASE(any_cast, bad_any_cast_inherits_exception)
{
    try
    {
        any a(1);
        any_cast<double>(static_cast<const any&>(a));
        EXPECT_TRUE(false);  // unreached
    }
    catch (const std::exception& _e)
    {
        EXPECT_TRUE(true);
        (void)_e;
    }
}
#endif
#endif
