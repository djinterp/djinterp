/******************************************************************************
* djinterp [test]                                      compose_tests_fix.cpp
*
*   Tests for compose.hpp Section VII: fix, the Y-combinator for
* recursive lambdas.  The combinator itself is available on the C++11
* baseline, but the ergonomic call form passes a generic ([](auto&
* self, ...)) lambda, which is a C++14 feature.  This file therefore
* provides a C++11-safe functor-based recursion test that runs on every
* standard, plus richer generic-lambda tests gated to C++14+.
******************************************************************************/

#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;


namespace {

    // factorial_fn
    //   struct: C++11-compatible recursion body for fix.  fix passes the
    // bound fix_helper as the first argument (_self); the templated
    // operator() accepts it by deduced reference so the same functor
    // works on every standard without a generic lambda.
    struct factorial_fn
    {
        template<typename _Self>
        int operator()(_Self& _self, int _n) const
        {
            return (_n <= 1) ? 1 : (_n * _self(_n - 1));
        }
    };

}  // namespace


/*
compose_tests_fix
  Exercises compose.hpp Section VII (fix).
  Tests the following:
  - a recursive factorial via a C++11-safe functor body (all standards)
  - the factorial base case (n <= 1) returns 1
  - a recursive factorial via a generic lambda (C++14+)
  - a recursive Fibonacci, to confirm the combinator is generic (C++14+)
  - multi-argument recursion (Euclid's gcd) through fix (C++14+)
*/
void
compose_tests_fix(
    test_handler& _handler
)
{
    unit_test_tally tally;

    // C++11-safe functor recursion -- runs on every standard
    run_unit_test(
        _handler,
        tally,
        "fix expresses recursion via a functor body",
        [&]()
        {
            auto factorial = ::djinterp::fix(factorial_fn());

            record_assertion(
                _handler,
                (factorial(5) == 120),
                "fix(factorial_fn)(5) == 120");

            record_assertion(
                _handler,
                (factorial(0) == 1),
                "fix(factorial_fn)(0) == 1 (base case)");

            record_assertion(
                _handler,
                (factorial(1) == 1),
                "fix(factorial_fn)(1) == 1 (base case)");
        });

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // recursive factorial via a generic lambda
    run_unit_test(
        _handler,
        tally,
        "fix expresses a recursive factorial (generic lambda)",
        [&]()
        {
            auto factorial = ::djinterp::fix(
                [](auto& _self, int _n) -> int
                {
                    return (_n <= 1) ? 1 : (_n * _self(_n - 1));
                });

            record_assertion(
                _handler,
                (factorial(6) == 720),
                "fix factorial(6) == 720");
        });

    // recursive Fibonacci -- a different shape, to show genericity
    run_unit_test(
        _handler,
        tally,
        "fix expresses a recursive Fibonacci (generic lambda)",
        [&]()
        {
            auto fib = ::djinterp::fix(
                [](auto& _self, int _n) -> int
                {
                    return (_n < 2) ? _n : (_self(_n - 1) + _self(_n - 2));
                });

            record_assertion(
                _handler,
                (fib(10) == 55),
                "fix fib(10) == 55");

            record_assertion(
                _handler,
                (fib(0) == 0 && fib(1) == 1),
                "fix fib base cases (0 -> 0, 1 -> 1)");
        });

    // multi-argument recursion: Euclid's gcd
    run_unit_test(
        _handler,
        tally,
        "fix supports multi-argument recursion (generic lambda)",
        [&]()
        {
            auto gcd = ::djinterp::fix(
                [](auto& _self, int _a, int _b) -> int
                {
                    return (_b == 0) ? _a : _self(_b, _a % _b);
                });

            record_assertion(
                _handler,
                (gcd(48, 18) == 6),
                "fix gcd(48, 18) == 6");

            record_assertion(
                _handler,
                (gcd(17, 5) == 1),
                "fix gcd(17, 5) == 1 (coprime)");
        });
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    return;
}


NS_END  // testing
NS_END  // djinterp
