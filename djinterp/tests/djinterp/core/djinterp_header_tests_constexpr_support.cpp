/******************************************************************************
* djinterp [test]                  djinterp_header_tests_constexpr_support.cpp
*
*   Section II tests: constexpr / noexcept support macros.
******************************************************************************/

#include "./djinterp_header_tests.hpp"


// D_INTERNAL_TEST_CONSTEXPR_ACTIVE
//   macro: mirrors the header's own gate -- 1 when D_CONSTEXPR truly expands
// to `constexpr`, 0 when D_TESTING_CONSTEXPR has stripped it for runtime
// instrumentation.  Compile-time (static_assert) checks are taken only when
// this is 1; the runtime checks always run.
#if ( defined(D_TESTING_CONSTEXPR) &&  \
      (D_TESTING_CONSTEXPR == 1) )
    #define D_INTERNAL_TEST_CONSTEXPR_ACTIVE  0
#else
    #define D_INTERNAL_TEST_CONSTEXPR_ACTIVE  1
#endif


namespace
{

    // d_constexpr_probe
    //   helper: a D_CONSTEXPR function used to confirm the qualifier yields a
    // value usable in a constant expression when constexpr is active.
    D_CONSTEXPR int
    d_constexpr_probe()
    {
        return 42;
    }

    // d_static_constexpr_probe
    //   helper: a D_STATIC_CONSTEXPR namespace-scope constant.
    D_STATIC_CONSTEXPR int d_static_constexpr_probe = 7;

    // d_constexpr_inline_probe
    //   helper: a D_CONSTEXPR_INLINE function.
    D_CONSTEXPR_INLINE int
    d_constexpr_inline_probe()
    {
        return 9;
    }

    // d_static_constexpr_inline_probe
    //   helper: a D_STATIC_CONSTEXPR_INLINE function.
    D_STATIC_CONSTEXPR_INLINE int
    d_static_constexpr_inline_probe()
    {
        return 11;
    }

    // d_noexcept_probe
    //   helper: a function carrying the D_NOEXCEPT specifier.
    void
    d_noexcept_probe() D_NOEXCEPT
    {
        return;
    }

    // d_throwing_probe
    //   helper: a function with no exception specifier (noexcept(false)),
    // used to confirm the noexcept operator actually discriminates.
    int
    d_throwing_probe()
    {
        return 0;
    }

}  // anonymous namespace


NS_DJINTERP
NS_TEST


/*
tests_constexpr_macros
  Verifies the constexpr-family qualifier macros compose and behave correctly.
  Tests the following:
  - D_CONSTEXPR yields a constant-expression-usable function (compile time,
    when constexpr is active) and the expected value (runtime)
  - D_STATIC_CONSTEXPR yields an internal-linkage compile-time constant of the
    expected value
  - D_CONSTEXPR_INLINE composes constexpr + inline and yields the expected
    value
  - D_STATIC_CONSTEXPR_INLINE composes static + constexpr + inline and yields
    the expected value
  - all compile-time assertions are gated on D_INTERNAL_TEST_CONSTEXPR_ACTIVE
    so the suite still builds under D_TESTING_CONSTEXPR=1
*/
bool
tests_constexpr_macros()
{
    bool ok = true;

#if D_INTERNAL_TEST_CONSTEXPR_ACTIVE
    // when constexpr is genuinely in effect, each probe is a constant
    // expression and may drive a static_assert.
    static_assert((d_constexpr_probe() == 42),
                  "D_CONSTEXPR must permit constant-expression evaluation.");
    static_assert((d_static_constexpr_probe == 7),
                  "D_STATIC_CONSTEXPR must yield a compile-time constant.");
    static_assert((d_constexpr_inline_probe() == 9),
                  "D_CONSTEXPR_INLINE must permit constant evaluation.");
    static_assert((d_static_constexpr_inline_probe() == 11),
                  "D_STATIC_CONSTEXPR_INLINE must permit constant "
                  "evaluation.");
#endif  // D_INTERNAL_TEST_CONSTEXPR_ACTIVE

    // runtime mirror -- valid whether or not constexpr was stripped.
    ok = ok && (d_constexpr_probe()              == 42);
    ok = ok && (d_static_constexpr_probe         == 7);
    ok = ok && (d_constexpr_inline_probe()       == 9);
    ok = ok && (d_static_constexpr_inline_probe() == 11);

    return ok;
}


/*
tests_noexcept_macro
  Verifies the portable D_NOEXCEPT specifier.
  Tests the following:
  - a function declared with D_NOEXCEPT is reported non-throwing by the
    noexcept operator (C++11 and later, which is the suite's floor)
  - a function with no specifier is reported potentially-throwing, confirming
    the operator is actually discriminating rather than always-true
*/
bool
tests_noexcept_macro()
{
    bool ok = true;

    // on every standard the suite targets (C++11+), D_NOEXCEPT == noexcept,
    // so the operator must report the probe as non-throwing.
    static_assert(noexcept(d_noexcept_probe()),
                  "D_NOEXCEPT must mark the function non-throwing.");
    static_assert(!noexcept(d_throwing_probe()),
                  "an unspecified function must remain potentially-throwing.");

    ok = ok && noexcept(d_noexcept_probe());
    ok = ok && (!noexcept(d_throwing_probe()));

    return ok;
}


NS_END  // test
NS_END  // djinterp
