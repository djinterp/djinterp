// djinterp [test] -- curry.hpp Section V (argument transformations)
#include "./curry_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_flip
  Exercises flip() and both operator() overloads of flip_helper.
  Tests the following:
  - the two-argument overload swaps its arguments (order-sensitive subtraction)
  - the documented example flip(sub)(3, 10) == 7
  - the three-or-more overload swaps only the first two, passing the rest
    through unchanged (ternary callable, empty remainder pack)
  - the same overload with a non-empty remainder pack (quaternary callable)
  - flipping is involutive: flip(flip(f))(a, b) == f(a, b)
*/
void
test_flip(
    test::test_handler& _h
)
{
    sub2 sub;

    // two-argument swap (documented example: 10 - 3)
    test::record_assertion(_h, (flip(sub)(3, 10) == 7),
                           "flip: swaps two args (10 - 3 == 7)");
    test::record_assertion(_h, (flip(sub)(10, 3) == -7),
                           "flip: swaps two args (3 - 10 == -7)");

    // three-argument: swap first two only, empty remainder pack
    test::record_assertion(_h, (flip(digits3{})(1, 2, 3) == 213),
                           "flip: ternary swaps first two only");

    // four-argument: swap first two, non-empty remainder pack passes through
    auto digits4 = [](int _a, int _b, int _c, int _d) {
        return (_a * 1000) + (_b * 100) + (_c * 10) + _d;
    };
    test::record_assertion(_h, (flip(digits4)(1, 2, 3, 4) == 2134),
                           "flip: quaternary swaps first two, rest pass through");

    // involution: flipping twice restores original order
    test::record_assertion(_h, (flip(flip(sub))(10, 3) == 7),
                           "flip: flip(flip(f)) restores argument order");

    return;
}


NS_END  // testing
NS_END  // djinterp
