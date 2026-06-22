// std
#include <type_traits>
// djinterp
#include "test_common_tests.hpp"


// Both macros must be defined by test_common.hpp regardless of language level
// (empty under C++11, a real qualifier under C++14+).
#ifndef D_TEST_CONSTEXPR
#  error "D_TEST_CONSTEXPR must be defined by test_common.hpp"
#endif
#ifndef D_TEST_STATIC_CONSTEXPR
#  error "D_TEST_STATIC_CONSTEXPR must be defined by test_common.hpp"
#endif


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


namespace
{
    // td_triangular
    //   helper: nth triangular number, written with a mutable local and a
    // loop.  Those constructs are illegal inside a C++11 constexpr function,
    // so the mere fact that this compiles under -std=c++11 proves that
    // D_TEST_CONSTEXPR expands to nothing there.  Under C++14+ it expands to
    // constexpr and the function becomes usable in a constant expression.
    D_TEST_CONSTEXPR int
    td_triangular(
        int _n
    )
    {
        int sum = 0;
        for (int i = 1; i <= _n; ++i)
        {
            sum += i;
        }
        return sum;
    }

    // a namespace-scope constant declared through the composed qualifier
    D_TEST_STATIC_CONSTEXPR int k_td_answer = 42;
}  // namespace


// On C++14+ both forms must produce genuine constant expressions.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(td_triangular(5) == 15,
              "D_TEST_CONSTEXPR must yield a constexpr function on C++14+");
static_assert(td_triangular(0) == 0,
              "D_TEST_CONSTEXPR function must constant-evaluate the base case");
static_assert(k_td_answer == 42,
              "D_TEST_STATIC_CONSTEXPR must yield a constexpr constant "
              "on C++14+");
#endif


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- IV. CONSTEXPR SUPPORT MACROS                           ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_d_test_constexpr
  Verifies the conditional-constexpr function qualifier.
  Tests the following:
  - a function carrying the qualifier returns correct results at runtime on
    every supported language level
  - on C++14+ that function is usable in a constant expression (proven by
    feeding its result to a non-type template parameter)
  - on C++11 the qualifier is inert: the relaxed-constexpr body above compiles
    as an ordinary function (a structural, compile-time guarantee)
*/
bool
tests_d_test_constexpr()
{
    bool ok = true;

    // runtime results are correct on every standard
    ok = D_TC_CHECK(td_triangular(0)  == 0)  && ok;
    ok = D_TC_CHECK(td_triangular(1)  == 1)  && ok;
    ok = D_TC_CHECK(td_triangular(5)  == 15) && ok;
    ok = D_TC_CHECK(td_triangular(10) == 55) && ok;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // the result is a core constant expression: usable as a non-type template
    // argument, and bindable to a constexpr object.
    ok = D_TC_CHECK(std::integral_constant<int, td_triangular(4)>::value == 10)
         && ok;
    {
        D_CONSTEXPR int folded = td_triangular(6);
        ok = D_TC_CHECK(folded == 21) && ok;
    }
#endif

    return ok;
}


/*
tests_d_test_static_constexpr
  Verifies the composed static + conditional-constexpr qualifier.
  Tests the following:
  - a namespace-scope constant declared with it holds the correct value on
    every supported language level
  - the qualifier also applies to a block-scope (function-local) declaration
  - on C++14+ the constant is a core constant expression (proven by feeding it
    to a non-type template parameter)
*/
bool
tests_d_test_static_constexpr()
{
    bool ok = true;

    // namespace-scope constant: correct value everywhere
    ok = D_TC_CHECK(k_td_answer == 42) && ok;

    // block-scope declaration through the same qualifier
    {
        D_TEST_STATIC_CONSTEXPR int local_k = 7;
        ok = D_TC_CHECK(local_k == 7) && ok;
    }

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // the constant is a core constant expression
    ok = D_TC_CHECK(std::integral_constant<int, k_td_answer>::value == 42)
         && ok;
    {
        D_CONSTEXPR int mirror = k_td_answer;
        ok = D_TC_CHECK(mirror == 42) && ok;
    }
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
