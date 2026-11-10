/*******************************************************************************
* djinterp [cpp]                                         curry_tests_uncurry.cpp
*
*
* path:      /tests/djinterp/cpp/functional/curry_tests_uncurry.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
// djinterp [test] -- curry.hpp Section IV (uncurrying)
#include "./curry_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_uncurry
  Exercises uncurry() and both operator() overloads of DUncurryHelper.
  Tests the following:
  - the two-argument path: uncurry(f)(a, b) applies f(a) then the result to b
  - the recursive multi-argument path with a non-empty remainder pack
    (three- and four-level curried callables)
  - the degenerate single-argument path over a plain (non-curried) unary
    callable, where the wrapper reduces to a direct call
  - equivalence with the hand-written curried form f(a)(b)(c)
*/
bool test_uncurry()
{
    bool verdict = true;

    // hand-rolled curried callables of increasing depth
    auto c2 = [](int _a) {
        return [_a](int _b) { return _a + _b; };
    };
    auto c3 = [](int _a) {
        return [_a](int _b) {
            return [_a, _b](int _c) { return _a + _b + _c; };
        };
    };
    auto c4 = [](int _a) {
        return [_a](int _b) {
            return [_a, _b](int _c) {
                return [_a, _b, _c](int _d) { return _a + _b + _c + _d; };
            };
        };
    };

    // two-argument path
    record_assertion(verdict, (uncurry(c2)(3, 4) == 7),
                           "uncurry: two-argument application");

    // recursive path, remainder pack {c}
    record_assertion(verdict, (uncurry(c3)(1, 2, 3) == 6),
                           "uncurry: three-argument recursion");

    // deeper recursion, remainder pack non-empty across steps
    record_assertion(verdict, (uncurry(c4)(1, 2, 3, 4) == 10),
                           "uncurry: four-argument recursion");

    // equivalence with explicit curried application
    record_assertion(verdict, (uncurry(c3)(1, 2, 3) == c3(1)(2)(3)),
                           "uncurry: matches f(a)(b)(c)");

    // degenerate single-argument path over a plain unary callable
    record_assertion(verdict, (uncurry(DEchoInt{})(41) == 41),
                           "uncurry: degenerate single-argument call");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
