/******************************************************************************
* djinterp [test]                                   comonad_tests_extract.cpp
*
*   Section II.1 of the comonad.hpp suite: extract, the counit, which reads the
* focus value out of a comonad (W<A> -> A).  Covers reading the focus over the
* std::pair and kv_pair Env instances and the custom Identity comonad, the fact
* that extract depends ONLY on the focus (the environment is ignored), a focus
* of a type different from the environment's, argument forwarding, and the
* by-value result type.
*
* path:      /tests/djinterp/core/functional/comonad_tests_extract.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "comonad_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_extract_pair
  Tests the following:
  - extract reads the focus (the second component) of a pair.
*/
static bool
tests_extract_pair()
{
    const std::pair<std::string, int> w("ctx", 10);

    return (::djinterp::extract(w) == 10);
}

/*
tests_extract_pair_env_ignored
  Tests the following:
  - extract depends only on the focus: two pairs with the same focus but
    different environments extract to the same value.
*/
static bool
tests_extract_pair_env_ignored()
{
    const std::pair<std::string, int> a("x", 5);
    const std::pair<std::string, int> b("yyyy", 5);

    return ( (::djinterp::extract(a) == 5) &&
             (::djinterp::extract(b) == 5) );
}

/*
tests_extract_pair_focus_type
  Tests the following:
  - the focus is the second component regardless of type: extract of
    pair<int,string> reads the string.
*/
static bool
tests_extract_pair_focus_type()
{
    const std::pair<int, std::string> w(1, std::string("hi"));

    return (::djinterp::extract(w) == "hi");
}

/*
tests_extract_kv
  Tests the following:
  - extract reads the value (the focus) of a kv_pair.
*/
static bool
tests_extract_kv()
{
    const ::djinterp::kv_pair<int, int> w(3, 42);

    return (::djinterp::extract(w) == 42);
}

/*
tests_extract_ident
  Tests the following:
  - extract reads the focus of the Identity comonad.
*/
static bool
tests_extract_ident()
{
    const ident<int> w(7);

    return (::djinterp::extract(w) == 7);
}

/*
tests_extract_forwarding
  Tests the following:
  - extract accepts both a named lvalue comonad and an rvalue (temporary) one.
*/
static bool
tests_extract_forwarding()
{
    std::pair<std::string, int> w("ctx", 10);

    const int from_lvalue = ::djinterp::extract(w);
    const int from_rvalue =
        ::djinterp::extract(std::pair<std::string, int>("ctx", 10));

    return ( (from_lvalue == 10) &&
             (from_rvalue == 10) );
}

/*
tests_extract_result_type
  Tests the following:
  - extract yields the focus by value (A, not a reference).
*/
static bool
tests_extract_result_type()
{
    return std::is_same<
        decltype(::djinterp::extract(
            std::declval< const std::pair<std::string, int>& >())),
        int >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
comonad_extract_block()
{
    dt::block_spec block;

    block.name       = "II.1 extract";
    block.descriptor =
        "read the focus: over both instances + custom, env ignored, result type";

    block.tests.push_back(dt::test_spec{
        "pair focus",
        "extract reads the second component",
        &tests_extract_pair });

    block.tests.push_back(dt::test_spec{
        "env ignored",
        "same focus, different env -> same extract",
        &tests_extract_pair_env_ignored });

    block.tests.push_back(dt::test_spec{
        "focus type independence",
        "focus is the second component whatever its type",
        &tests_extract_pair_focus_type });

    block.tests.push_back(dt::test_spec{
        "kv focus",
        "extract reads the value",
        &tests_extract_kv });

    block.tests.push_back(dt::test_spec{
        "ident focus",
        "extract reads the Identity focus",
        &tests_extract_ident });

    block.tests.push_back(dt::test_spec{
        "forwarding",
        "lvalue and rvalue comonad operands",
        &tests_extract_forwarding });

    block.tests.push_back(dt::test_spec{
        "result type",
        "focus returned by value",
        &tests_extract_result_type });

    return block;
}


NS_END  // testing
NS_END  // djinterp
